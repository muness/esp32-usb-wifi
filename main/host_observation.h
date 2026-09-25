/* SPDX-License-Identifier: MIT
 * Passive source observations only; these do not establish DHCP success. */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct {
    uint8_t ipv4[4], ipv6[16];
    bool valid4, valid6;
    uint64_t seen4_ms, seen6_ms;
} host_observation_t;
void host_observe_frame(host_observation_t *state, const uint8_t *frame, size_t len, uint64_t now_ms);

#define HOST_OBSERVATION_TTL_MS 60000u
void host_observation_clear(host_observation_t *state);
bool host_observed_ipv4(const host_observation_t *state, uint8_t ip[4], uint64_t now_ms);
bool host_observed_ipv6(const host_observation_t *state, uint8_t ip[16], uint64_t now_ms);
