#ifndef RING_BUFFER_HPP
#define RING_BUFFER_HPP

#include <vector>
#include <atomic>
#include <cstdint>

template<typename T>
class RingBuffer {
public:
    explicit RingBuffer(size_t capacity) : _capacity(capacity), _buffer(capacity) {
        _head.store(0, std::memory_order_relaxed);
        _tail.store(0, std::memory_order_relaxed);
    }

    // Producer (Network Thread): Push data into the buffer
    bool push(const T& item) {
        const size_t head = _head.load(std::memory_order_relaxed);
        const size_t next_head = (head + 1) % _capacity;

        // Check if buffer is full
        if (next_head == _tail.load(std::memory_order_acquire)) {
            return false;
        }

        _buffer[head] = item;
        _head.store(next_head, std::memory_order_release);
        return true;
    }

    // Consumer (Disk/Logging Thread): Pop data from the buffer
    bool pop(T& item) {
        const size_t tail = _tail.load(std::memory_order_relaxed);

        // Check if buffer is empty
        if (tail == _head.load(std::memory_order_acquire)) {
            return false;
        }

        item = _buffer[tail];
        _tail.store((tail + 1) % _capacity, std::memory_order_release);
        return true;
    }

private:
    size_t _capacity;
    std::vector<T> _buffer;
    alignas(64) std::atomic<size_t> _head; // alignas prevents false sharing
    alignas(64) std::atomic<size_t> _tail;
};

#endif
