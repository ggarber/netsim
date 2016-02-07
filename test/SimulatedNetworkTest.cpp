#include <gtest/gtest.h>

#include "SimulatedNetwork.hpp"

using netsim::Config;
using netsim::SimulatedNetwork;

class SimulatedNetworkTest: public ::testing::Test {
};

TEST_F(SimulatedNetworkTest, BasicTest) {
		Config config;
		config.link_capacity_kbps   = 500;
		config.queue_length_packets = 50;
		config.loss_percent         = 0;
		SimulatedNetwork network(config);

    network.EnqueuePacket({ 1000, 0, 1 });

    auto delivery_time = network.NextDeliveryTimeUs();
    ASSERT_EQ(16000, *delivery_time);

    auto deliverable_packets = network.DequeueDeliverablePackets(*delivery_time);
    ASSERT_EQ(1, deliverable_packets.size());
    ASSERT_EQ(1, deliverable_packets[0].packet_id);
}
