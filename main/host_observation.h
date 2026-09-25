/* SPDX-License-Identifier: MIT
 * Passive source observations only; these do not establish DHCP success. */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
  HOST_OBSERVATION_TTL_SECONDS = 60,
  HOST_MILLISECONDS_PER_SECOND = 1000,
};

#define HOST_OBSERVATION_TTL_MS                                                \
  (HOST_OBSERVATION_TTL_SECONDS * HOST_MILLISECONDS_PER_SECOND)

typedef struct {
  uint8_t ipv4[4];
  uint8_t ipv6[16];
  bool valid4;
  bool valid6;
  uint64_t seen4_ms;
  uint64_t seen6_ms;
} host_observation_t;

/* Record valid source addresses from a complete Ethernet frame at now_ms. */
void host_observe_frame(host_observation_t *state, const uint8_t *frame,
                        size_t frame_len, uint64_t now_ms);

/* Invalidate all observations when the link or USB host changes. */
void host_observation_clear(host_observation_t *state);

/* Copy a fresh observation to ip; return false when absent or expired. */
bool host_observed_ipv4(const host_observation_t *state, uint8_t ip[4],
                        uint64_t now_ms);
bool host_observed_ipv6(const host_observation_t *state, uint8_t ip[16],
                        uint64_t now_ms);
