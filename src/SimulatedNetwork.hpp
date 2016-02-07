#ifndef NETSIM_SIMULATED_NETWORK_HPP_
#define NETSIM_SIMULATED_NETWORK_HPP_

#include <string>
#include <vector>
#include <queue>
#include <deque>
#include <utility>

#include "Random.hpp"

namespace netsim {

// templatized struct to encapsulate an optional value without using std::optional
// (which is not available in C++11)
template <typename T>
struct optional {
  optional() : has_value_(false) {}
  optional(const T& value) : value_(value), has_value_(true) {}
  optional(T&& value) : value_(std::move(value)), has_value_(true) {}
  optional(const optional& other) : value_(other.value_), has_value_(other.has_value_) {}
  optional(optional&& other) : value_(std::move(other.value_)), has_value_(other.has_value_) {}
  optional& operator=(const optional& other) {
    value_ = other.value_;
    has_value_ = other.has_value_;
    return *this;
  }
  optional& operator=(optional&& other) {
    value_ = std::move(other.value_);
    has_value_ = other.has_value_;
    return *this;
  }
  void reset() { has_value_ = false; }
  bool has_value() const { return has_value_; }
  const T& value() const { return value_; }
  T& value() { return value_; }
  const T& operator*() const { return value_; }
  T& operator*() { return value_; }
  const T* operator->() const { return &value_; }
  T* operator->() { return &value_; }
  operator bool() const { return has_value_; }

 private:
  T value_;
  bool has_value_;
};

struct Config {
  //  Queue length in number of packets.
  size_t queue_length_packets = 0;
  // Delay in addition to capacity induced delay.
  int queue_delay_ms = 0;
  // Standard deviation of the extra delay.
  int delay_standard_deviation_ms = 0;
  // Link capacity in kbps.
  int link_capacity_kbps = 0;
  // Random packet loss.
  int loss_percent = 0;
  // If packets are allowed to be reordered.
  bool allow_reordering = false;
  // The average length of a burst of lost packets.
  int avg_burst_loss_length = -1;
  // Additional bytes to add to packet size.
  int packet_overhead = 0;
};

struct PacketInFlightInfo {
  PacketInFlightInfo(size_t size, int64_t send_time_us, uint64_t packet_id)
      : size(size), send_time_us(send_time_us), packet_id(packet_id) {}

  size_t size;
  int64_t send_time_us;
  // Unique identifier for the packet in relation to other packets in flight.
  uint64_t packet_id;
};

struct PacketDeliveryInfo {
  static constexpr int kNotReceived = -1;
  PacketDeliveryInfo(PacketInFlightInfo source, int64_t receive_time_us)
      : receive_time_us(receive_time_us), packet_id(source.packet_id) {}

  bool operator==(const PacketDeliveryInfo& other) const {
    return receive_time_us == other.receive_time_us &&
           packet_id == other.packet_id;
  }

  int64_t receive_time_us;
  uint64_t packet_id;
};

// Class simulating a network link.
//
// This is a basic implementation of NetworkBehaviorInterface that supports:
// - Packet loss
// - Capacity delay
// - Extra delay with or without packets reorder
// - Packet overhead
// - Queue max capacity
class SimulatedNetwork {
 public:
  explicit SimulatedNetwork(Config config, uint64_t random_seed = 1);
  ~SimulatedNetwork();

  // Sets a new configuration. This will affect packets that will be sent with
  // EnqueuePacket but also packets in the network that have not left the
  // network emulation. Packets that are ready to be retrieved by
  // DequeueDeliverablePackets are not affected by the new configuration.
  // TODO(bugs.webrtc.org/14525): Fix SetConfig and make it apply only to the
  // part of the packet that is currently being sent (instead of applying to
  // all of it).
  void SetConfig(const Config& config);
  void UpdateConfig(std::function<void(Config*)>
                        config_modifier);
  void PauseTransmissionUntil(int64_t until_us);

  // NetworkBehaviorInterface
  bool EnqueuePacket(PacketInFlightInfo packet);
  std::vector<PacketDeliveryInfo> DequeueDeliverablePackets(
      int64_t receive_time_us);

  optional<int64_t> NextDeliveryTimeUs() const;

 private:
  struct PacketInfo {
    PacketInFlightInfo packet;
    // Time when the packet has left (or will leave) the network.
    int64_t arrival_time_us;
  };
  // Contains current configuration state.
  struct ConfigState {
    // Static link configuration.
    Config config;
    // The probability to drop the packet if we are currently dropping a
    // burst of packet
    double prob_loss_bursting;
    // The probability to drop a burst of packets.
    double prob_start_bursting;
    // Used for temporary delay spikes.
    int64_t pause_transmission_until_us = 0;
  };

  // Moves packets from capacity- to delay link.
  void UpdateCapacityQueue(ConfigState state, int64_t time_now_us);
  ConfigState GetConfigState() const;

  // Models the capacity of the network by rejecting packets if the queue is
  // full and keeping them in the queue until they are ready to exit (according
  // to the link capacity, which cannot be violated, e.g. a 1 kbps link will
  // only be able to deliver 1000 bits per second).
  //
  // Invariant:
  // The head of the `capacity_link_` has arrival_time_us correctly set to the
  // time when the packet is supposed to be delivered (without accounting
  // potential packet loss or potential extra delay and without accounting for a
  // new configuration of the network, which requires a re-computation of the
  // arrival_time_us).
  std::queue<PacketInfo> capacity_link_;
  // Models the extra delay of the network (see `queue_delay_ms`
  // and `delay_standard_deviation_ms` in BuiltInNetworkBehaviorConfig), packets
  // in the `delay_link_` have technically already left the network and don't
  // use its capacity but they are not delivered yet.
  std::deque<PacketInfo> delay_link_;
  // Represents the next moment in time when the network is supposed to deliver
  // packets to the client (either by pulling them from `delay_link_` or
  // `capacity_link_` or both).
  optional<int64_t> next_process_time_us_;

  ConfigState config_state_;

  Random random_;

  // Are we currently dropping a burst of packets?
  bool bursting_;

  // The send time of the last enqueued packet, this is only used to check that
  // the send time of enqueued packets is monotonically increasing.
  int64_t last_enqueue_time_us_;

  // The last time a packet left the capacity_link_ (used to enforce
  // the capacity of the link and avoid packets starts to get sent before
  // the link it free).
  int64_t last_capacity_link_exit_time_;
};

}  // namespace netsim

#endif  // NETSIM_SIMULATED_NETWORK_HPP_
