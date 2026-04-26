# Use an Ubuntu base for high-performance networking tools
FROM ubuntu:22.04

# Install build essentials and python
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    g++ \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Install Python dependencies
RUN pip3 install pandas pyarrow fastparquet

# Set up project directory
WORKDIR /app
COPY . .

# Compile the C++ Recorder
RUN g++ -O3 -pthread src/recorder.cpp -o chronos_recorder -I./include

# Default command: start the recorder
CMD ["./chronos_recorder"]
