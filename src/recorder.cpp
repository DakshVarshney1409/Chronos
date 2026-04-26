#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <chrono>
#include "ring_buffer.hpp"

struct MarketPacket {
    uint64_t timestamp;
    uint32_t seq_num;
    char data[64];
};

int main() {
    const char* mcast_addr = "239.1.1.1";
    const int port = 12345;
    
    // Initialize Lock-Free Buffer
    RingBuffer<MarketPacket> queue(65536);

    // UDP Socket Setup
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    bind(fd, (struct sockaddr*)&addr, sizeof(addr));

    // Join Multicast Group
    ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(mcast_addr);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

    std::cout << "Chronos Recorder listening on " << mcast_addr << ":" << port << std::endl;

    // Background Thread: Write data to disk to avoid blocking the network thread
    std::thread writer_thread([&queue]() {
        FILE* fp = fopen("raw_market_data.bin", "wb");
        MarketPacket pkt;
        while (true) {
            if (queue.pop(pkt)) {
                fwrite(&pkt, sizeof(MarketPacket), 1, fp);
            } else {
                std::this_thread::yield(); // Don't burn CPU if empty
            }
        }
        fclose(fp);
    });

    // Main Thread: High-priority Network Capture
    MarketPacket incoming;
    while (true) {
        ssize_t len = recv(fd, incoming.data, sizeof(incoming.data), 0);
        if (len > 0) {
            incoming.timestamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            if (!queue.push(incoming)) {
                std::cerr << "Buffer Overflow! Packet dropped." << std::endl;
            }
        }
    }

    return 0;
}
