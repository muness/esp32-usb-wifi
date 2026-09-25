/* SPDX-License-Identifier: MIT */
#include "host_observation.h"

#include <string.h>

/* Ethernet II fields use offsets from the beginning of the complete frame. */
enum {
  ETHERNET_HEADER_SIZE = 14,
  ETHERNET_TYPE_OFFSET = 12,
  ETHERNET_TYPE_ARP = 0x0806,
  ETHERNET_TYPE_IPV4 = 0x0800,
  ETHERNET_TYPE_IPV6 = 0x86dd,
};

/* ARP fields, also expressed as offsets from the beginning of the frame. */
enum {
  ARP_PACKET_SIZE =
      28, /* Ethernet/IPv4 ARP fields after the Ethernet header. */
  ARP_FRAME_SIZE = ETHERNET_HEADER_SIZE + ARP_PACKET_SIZE,
  ARP_HARDWARE_TYPE_OFFSET = 14,
  ARP_PROTOCOL_TYPE_OFFSET = 16,
  ARP_HARDWARE_LENGTH_OFFSET = 18,
  ARP_PROTOCOL_LENGTH_OFFSET = 19,
  ARP_OPERATION_OFFSET = 20,
  ARP_SENDER_IPV4_OFFSET = 28,
  ARP_ETHERNET_HARDWARE_TYPE = 1,
  ARP_IPV4_PROTOCOL_TYPE = 0x0800,
  ARP_ETHERNET_ADDRESS_SIZE = 6,
  ARP_IPV4_ADDRESS_SIZE = 4,
  ARP_REQUEST = 1,
  ARP_REPLY = 2,
};

/* IPv4 fields and minimum header size. IHL may include additional options. */
enum {
  IPV4_MINIMUM_HEADER_SIZE = 20,
  IPV4_MINIMUM_FRAME_SIZE = ETHERNET_HEADER_SIZE + IPV4_MINIMUM_HEADER_SIZE,
  IPV4_VERSION_AND_IHL_OFFSET = 14,
  IPV4_VERSION_SHIFT = 4,
  IPV4_VERSION = 4,
  IPV4_IHL_MASK = 0x0f,
  IPV4_TOTAL_LENGTH_OFFSET = 16,
  IPV4_SOURCE_ADDRESS_OFFSET = 26,
};

/* The IPv6 source follows its fixed 40-byte header. */
enum {
  IPV6_HEADER_SIZE = 40,
  IPV6_MINIMUM_FRAME_SIZE = ETHERNET_HEADER_SIZE + IPV6_HEADER_SIZE,
  IPV6_VERSION_OFFSET = 14,
  IPV6_VERSION_SHIFT = 4,
  IPV6_VERSION = 6,
  IPV6_PAYLOAD_LENGTH_OFFSET = 18,
  IPV6_SOURCE_ADDRESS_OFFSET = 22,
  IPV6_GLOBAL_UNICAST_PREFIX_MASK = 0xe0,
  IPV6_GLOBAL_UNICAST_PREFIX = 0x20, /* 2000::/3, matching the donor policy */
};

static uint16_t read_big_endian_u16(const uint8_t *bytes) {
  return (uint16_t)(((uint16_t)bytes[0] << 8) | bytes[1]);
}

static bool is_supported_arp_frame(const uint8_t *frame, size_t frame_len) {
  if (frame_len < ARP_FRAME_SIZE) {
    return false;
  }

  const uint16_t hardware_type =
      read_big_endian_u16(frame + ARP_HARDWARE_TYPE_OFFSET);
  const uint16_t protocol_type =
      read_big_endian_u16(frame + ARP_PROTOCOL_TYPE_OFFSET);
  const uint16_t operation = read_big_endian_u16(frame + ARP_OPERATION_OFFSET);

  /* Accept only Ethernet/IPv4 ARP requests or replies with standard address
   * sizes. */
  return hardware_type == ARP_ETHERNET_HARDWARE_TYPE &&
         protocol_type == ARP_IPV4_PROTOCOL_TYPE &&
         frame[ARP_HARDWARE_LENGTH_OFFSET] == ARP_ETHERNET_ADDRESS_SIZE &&
         frame[ARP_PROTOCOL_LENGTH_OFFSET] == ARP_IPV4_ADDRESS_SIZE &&
         (operation == ARP_REQUEST || operation == ARP_REPLY);
}

static const uint8_t *ipv4_source_address(const uint8_t *frame,
                                          size_t frame_len) {
  if (frame_len < IPV4_MINIMUM_FRAME_SIZE) {
    return NULL;
  }

  const uint8_t version_and_ihl = frame[IPV4_VERSION_AND_IHL_OFFSET];
  const uint8_t version = version_and_ihl >> IPV4_VERSION_SHIFT;
  const size_t header_len = (version_and_ihl & IPV4_IHL_MASK) * 4u;
  const size_t packet_len =
      read_big_endian_u16(frame + IPV4_TOTAL_LENGTH_OFFSET);

  /* Reject non-IPv4 headers, impossible IHL/length values, and truncated
   * frames. */
  if (version != IPV4_VERSION || header_len < IPV4_MINIMUM_HEADER_SIZE ||
      packet_len < header_len ||
      packet_len > frame_len - ETHERNET_HEADER_SIZE) {
    return NULL;
  }

  return frame + IPV4_SOURCE_ADDRESS_OFFSET;
}

static const uint8_t *ipv6_global_source_address(const uint8_t *frame,
                                                 size_t frame_len) {
  if (frame_len < IPV6_MINIMUM_FRAME_SIZE) {
    return NULL;
  }

  const uint8_t version = frame[IPV6_VERSION_OFFSET] >> IPV6_VERSION_SHIFT;
  const size_t payload_len =
      read_big_endian_u16(frame + IPV6_PAYLOAD_LENGTH_OFFSET);

  /* The fixed header and its declared payload must both be present in the
   * frame. */
  if (version != IPV6_VERSION ||
      payload_len > frame_len - IPV6_MINIMUM_FRAME_SIZE) {
    return NULL;
  }

  const uint8_t *source = frame + IPV6_SOURCE_ADDRESS_OFFSET;
  if ((source[0] & IPV6_GLOBAL_UNICAST_PREFIX_MASK) !=
      IPV6_GLOBAL_UNICAST_PREFIX) {
    return NULL;
  }

  return source;
}

static bool is_nonzero_ipv4_address(const uint8_t address[4]) {
  return (address[0] | address[1] | address[2] | address[3]) != 0;
}

void host_observe_frame(host_observation_t *state, const uint8_t *frame,
                        size_t frame_len) {
  if (frame_len < ETHERNET_HEADER_SIZE) {
    return;
  }

  const uint16_t ether_type = read_big_endian_u16(frame + ETHERNET_TYPE_OFFSET);
  const uint8_t *ipv4_source = NULL;

  if (ether_type == ETHERNET_TYPE_ARP &&
      is_supported_arp_frame(frame, frame_len)) {
    /* ARP identifies its sender address in the sender-protocol-address field.
     */
    ipv4_source = frame + ARP_SENDER_IPV4_OFFSET;
  } else if (ether_type == ETHERNET_TYPE_IPV4) {
    ipv4_source = ipv4_source_address(frame, frame_len);
  } else if (ether_type == ETHERNET_TYPE_IPV6) {
    const uint8_t *ipv6_source = ipv6_global_source_address(frame, frame_len);
    if (ipv6_source != NULL) {
      memcpy(state->ipv6, ipv6_source, sizeof(state->ipv6));
      state->valid6 = true;
    }
  }

  if (ipv4_source != NULL && is_nonzero_ipv4_address(ipv4_source)) {
    memcpy(state->ipv4, ipv4_source, sizeof(state->ipv4));
    state->valid4 = true;
  }
}
