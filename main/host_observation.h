/* SPDX-License-Identifier: MIT
 * Passive source observations only; these do not establish DHCP success. */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct {
    uint8_t ipv4[4], ipv6[16];
    bool valid4, valid6;
} host_observation_t;
void host_observe_frame(host_observation_t *state, const uint8_t *frame, size_t len);
