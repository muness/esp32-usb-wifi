/* SPDX-License-Identifier: MIT */
#include "host_observation.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

enum {
  TEST_FRAME_CAPACITY = 128,
  ETHERNET_HEADER_SIZE = 14,
  ETHERNET_TYPE_OFFSET = 12,
  ETHERNET_PAYLOAD_OFFSET = ETHERNET_HEADER_SIZE,
  ETHERNET_TYPE_IPV4_HIGH = 0x08,
  ETHERNET_TYPE_IPV4_LOW = 0x00,
  ETHERNET_TYPE_ARP_HIGH = 0x08,
  ETHERNET_TYPE_ARP_LOW = 0x06,
  ETHERNET_TYPE_IPV6_HIGH = 0x86,
  ETHERNET_TYPE_IPV6_LOW = 0xdd,
  IPV4_VERSION_SHIFT = 4,
  IPV4_VERSION = 4,
  IPV4_IHL_WORDS = 5,
  IPV4_SHORT_IHL_WORDS = 4,
  IPV4_LONG_IHL_WORDS = 6,
  IPV4_VERSION_AND_IHL = (IPV4_VERSION << IPV4_VERSION_SHIFT) | IPV4_IHL_WORDS,
  IPV4_HEADER_SIZE = IPV4_IHL_WORDS * 4,
  IPV4_TOTAL_LENGTH_OFFSET = 16,
  IPV4_SOURCE_ADDRESS_OFFSET = 26,
  IPV4_SOURCE_LAST_OCTET_OFFSET = IPV4_SOURCE_ADDRESS_OFFSET + 3,
  ARP_HARDWARE_TYPE_OFFSET = ETHERNET_PAYLOAD_OFFSET,
  ARP_PROTOCOL_TYPE_OFFSET = ARP_HARDWARE_TYPE_OFFSET + 2,
  ARP_HARDWARE_LENGTH_OFFSET = ARP_PROTOCOL_TYPE_OFFSET + 2,
  ARP_PROTOCOL_LENGTH_OFFSET = ARP_HARDWARE_LENGTH_OFFSET + 1,
  ARP_OPERATION_OFFSET = 20,
  ARP_SENDER_IPV4_OFFSET = 28,
  ARP_PACKET_SIZE = 28,
  ARP_FULL_FRAME_SIZE = ETHERNET_HEADER_SIZE + ARP_PACKET_SIZE,
  ARP_ETHERNET_HARDWARE_TYPE = 1,
  ARP_ETHERNET_ADDRESS_SIZE = 6,
  ARP_IPV4_ADDRESS_SIZE = 4,
  ARP_REQUEST = 1,
  IPV6_PAYLOAD_LENGTH_OFFSET = 18,
  IPV6_SOURCE_ADDRESS_OFFSET = 22,
  IPV6_HEADER_SIZE = 40,
  IPV6_FULL_FRAME_SIZE = ETHERNET_HEADER_SIZE + IPV6_HEADER_SIZE,
  IPV6_VERSION_SHIFT = 4,
  IPV6_VERSION = 6,
  IPV6_VERSION_FIELD = IPV6_VERSION << IPV6_VERSION_SHIFT,
  IPV6_GLOBAL_UNICAST_FIRST_BYTE = 0x20,
  IPV6_LINK_LOCAL_FIRST_BYTE = 0xfe,
  TEST_OBSERVATION_TIME_MS = 1000,
  TEST_CLOCK_BEFORE_OBSERVATION_MS = TEST_OBSERVATION_TIME_MS - 1,
  TEST_OBSERVATION_BEFORE_EXPIRY_MS = TEST_OBSERVATION_TIME_MS +
      HOST_OBSERVATION_TTL_MS - 1,
  TEST_OBSERVATION_AT_EXPIRY_MS = TEST_OBSERVATION_TIME_MS +
      HOST_OBSERVATION_TTL_MS,
};

static void observe_frame_at_test_time(host_observation_t *state,
                                       const uint8_t *frame, size_t frame_len) {
  host_observe_frame(state, frame, frame_len, TEST_OBSERVATION_TIME_MS);
}

static void write_big_endian_u16(uint8_t *bytes, uint16_t value) {
  bytes[0] = (uint8_t)(value >> 8);
  bytes[1] = (uint8_t)value;
}

static void set_ether_type(uint8_t frame[TEST_FRAME_CAPACITY], uint8_t high,
                           uint8_t low) {
  frame[ETHERNET_TYPE_OFFSET] = high;
  frame[ETHERNET_TYPE_OFFSET + 1] = low;
}

static void make_ipv4_frame(uint8_t frame[TEST_FRAME_CAPACITY]) {
  memset(frame, 0, TEST_FRAME_CAPACITY);
  set_ether_type(frame, ETHERNET_TYPE_IPV4_HIGH, ETHERNET_TYPE_IPV4_LOW);

  /* Version 4, IHL 5 (20-byte header), total length 20, source 192.0.0.2. */
  frame[ETHERNET_PAYLOAD_OFFSET] = IPV4_VERSION_AND_IHL;
  write_big_endian_u16(frame + IPV4_TOTAL_LENGTH_OFFSET, IPV4_HEADER_SIZE);
  frame[IPV4_SOURCE_ADDRESS_OFFSET] = 192;
  frame[IPV4_SOURCE_LAST_OCTET_OFFSET] = 2;
}

static void test_ipv4_validation(void) {
  uint8_t frame[TEST_FRAME_CAPACITY];
  host_observation_t state = {0};
  make_ipv4_frame(frame);

  /* No prefix of the Ethernet and IPv4 headers is enough to learn an address.
   */
  for (size_t frame_len = 0; frame_len < 34; frame_len++) {
    observe_frame_at_test_time(&state, frame, frame_len);
    assert(!state.valid4);
  }

  observe_frame_at_test_time(&state, frame, 34);
  assert(state.valid4 && state.ipv4[0] == 192 && state.ipv4[3] == 2);

  /* Reject invalid IHL values and a total length larger than the received
   * frame. */
  state = (host_observation_t){0};
  frame[ETHERNET_PAYLOAD_OFFSET] =
      (IPV4_VERSION << IPV4_VERSION_SHIFT) | IPV4_SHORT_IHL_WORDS;
  observe_frame_at_test_time(&state, frame, 34);
  assert(!state.valid4);

  frame[ETHERNET_PAYLOAD_OFFSET] =
      (IPV4_VERSION << IPV4_VERSION_SHIFT) | IPV4_LONG_IHL_WORDS;
  observe_frame_at_test_time(&state, frame, 34);
  assert(!state.valid4);

  frame[ETHERNET_PAYLOAD_OFFSET] = IPV4_VERSION_AND_IHL;
  write_big_endian_u16(frame + IPV4_TOTAL_LENGTH_OFFSET, IPV4_HEADER_SIZE + 1);
  observe_frame_at_test_time(&state, frame, 34);
  assert(!state.valid4);
}

static void make_arp_frame(uint8_t frame[TEST_FRAME_CAPACITY]) {
  memset(frame, 0, TEST_FRAME_CAPACITY);
  set_ether_type(frame, ETHERNET_TYPE_ARP_HIGH, ETHERNET_TYPE_ARP_LOW);

  /* Ethernet hardware, IPv4 protocol, 6/4-byte addresses, operation=request. */
  write_big_endian_u16(frame + ARP_HARDWARE_TYPE_OFFSET,
                       ARP_ETHERNET_HARDWARE_TYPE);
  write_big_endian_u16(frame + ARP_PROTOCOL_TYPE_OFFSET,
                       (ETHERNET_TYPE_IPV4_HIGH << 8) | ETHERNET_TYPE_IPV4_LOW);
  frame[ARP_HARDWARE_LENGTH_OFFSET] = ARP_ETHERNET_ADDRESS_SIZE;
  frame[ARP_PROTOCOL_LENGTH_OFFSET] = ARP_IPV4_ADDRESS_SIZE;
  write_big_endian_u16(frame + ARP_OPERATION_OFFSET, ARP_REQUEST);
  frame[ARP_SENDER_IPV4_OFFSET] = 10;
}

static void test_arp_validation(void) {
  uint8_t frame[TEST_FRAME_CAPACITY];
  host_observation_t state = {0};
  make_arp_frame(frame);

  observe_frame_at_test_time(&state, frame, ARP_FULL_FRAME_SIZE - 1);
  assert(!state.valid4);

  observe_frame_at_test_time(&state, frame, ARP_FULL_FRAME_SIZE);
  assert(state.valid4 && state.ipv4[0] == 10);

  state = (host_observation_t){0};
  frame[ARP_HARDWARE_LENGTH_OFFSET] = 5;
  observe_frame_at_test_time(&state, frame, ARP_FULL_FRAME_SIZE);
  assert(!state.valid4);
}

static void test_ipv6_validation(void) {
  uint8_t frame[TEST_FRAME_CAPACITY] = {0};
  host_observation_t state = {0};
  set_ether_type(frame, ETHERNET_TYPE_IPV6_HIGH, ETHERNET_TYPE_IPV6_LOW);
  frame[ETHERNET_PAYLOAD_OFFSET] = IPV6_VERSION_FIELD;
  frame[IPV6_SOURCE_ADDRESS_OFFSET] = IPV6_GLOBAL_UNICAST_FIRST_BYTE;

  observe_frame_at_test_time(&state, frame, IPV6_FULL_FRAME_SIZE - 1);
  assert(!state.valid6);

  observe_frame_at_test_time(&state, frame, IPV6_FULL_FRAME_SIZE);
  assert(state.valid6);

  state = (host_observation_t){0};
  write_big_endian_u16(frame + IPV6_PAYLOAD_LENGTH_OFFSET, 1);
  observe_frame_at_test_time(&state, frame, IPV6_FULL_FRAME_SIZE);
  assert(!state.valid6);

  write_big_endian_u16(frame + IPV6_PAYLOAD_LENGTH_OFFSET, 0);
  frame[IPV6_SOURCE_ADDRESS_OFFSET] = IPV6_LINK_LOCAL_FIRST_BYTE;
  observe_frame_at_test_time(&state, frame, IPV6_FULL_FRAME_SIZE);
  assert(!state.valid6);
}

static void test_deterministic_malformed_frame_corpus(void) {
  uint8_t frame[TEST_FRAME_CAPACITY];
  host_observation_t state = {0};
  uint32_t random = 1;

  /* Exercise short and malformed inputs with a repeatable pseudo-random corpus.
   */
  for (unsigned test_case = 0; test_case < 10000; test_case++) {
    for (size_t byte = 0; byte < sizeof(frame); byte++) {
      random = random * 1664525u + 1013904223u;
      frame[byte] = (uint8_t)(random >> 24);
    }

    observe_frame_at_test_time(&state, frame, test_case % sizeof(frame));
  }
}

static void test_observation_expiry_and_clear(void) {
  host_observation_t state = {.valid4 = true,
                              .seen4_ms = TEST_OBSERVATION_TIME_MS};
  uint8_t address[16] = {0};

  assert(
      host_observed_ipv4(&state, address, TEST_OBSERVATION_BEFORE_EXPIRY_MS));
  assert(!host_observed_ipv4(&state, address, TEST_OBSERVATION_AT_EXPIRY_MS));
  assert(
      !host_observed_ipv4(&state, address, TEST_CLOCK_BEFORE_OBSERVATION_MS));

  state.valid6 = true;
  state.seen6_ms = TEST_OBSERVATION_TIME_MS;
  assert(host_observed_ipv6(&state, address, TEST_OBSERVATION_TIME_MS + 1));

  host_observation_clear(&state);
  assert(!host_observed_ipv4(&state, address, TEST_OBSERVATION_TIME_MS + 1));
  assert(!host_observed_ipv6(&state, address, TEST_OBSERVATION_TIME_MS + 1));
}

int main(void) {
  test_ipv4_validation();
  test_arp_validation();
  test_ipv6_validation();
  test_deterministic_malformed_frame_corpus();
  test_observation_expiry_and_clear();

  puts("PASS: packet bounds, malformed headers, donor IPv6 policy, and "
       "10000-input corpus");
}
