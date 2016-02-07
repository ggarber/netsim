# netsim
C++ library to simulate the behaviour of restricted networks in automated tests.   It allows to simulate networks with limited bandwidth and different types of packet loss.

The code is mostly taken from the libwebrtc implementation from Google (very minor changes): https://chromium.googlesource.com/external/webrtc/+/refs/heads/master/call/simulated_network.h

# How to use it

```
    Config config;
    config.link_capacity_kbps   = 500;
    config.queue_length_packets = 50;
    config.loss_percent         = 0;
    SimulatedNetwork network(config);

    // Send a packet to the simulated network with 1000 bytes and identifier 1
    network.EnqueuePacket({ 1000, 0, 1 });

    // Check with the simulated network when is time to receive the next
    // queued packet
    auto delivery_time = network.NextDeliveryTimeUs();

    // Dequeue that packet
    auto deliverable_packets = network.DequeueDeliverablePackets(*delivery_time);
```

# Building
The project generates a static library that can be linked in other projects:
```
cd build
cmake ..
make -j 8
```
In some cases it make 