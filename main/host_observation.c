/* SPDX-License-Identifier: MIT */
#include "host_observation.h"
#include <string.h>
void host_observe_frame(host_observation_t *s, const uint8_t *f, size_t n)
{
    if (n < 14) return;
    const unsigned eth = ((unsigned)f[12] << 8) | f[13];
    const uint8_t *ip = NULL;
    if (eth == 0x0806 && n >= 42 && f[14] == 0 && f[15] == 1 &&
        f[16] == 8 && f[17] == 0 && f[18] == 6 && f[19] == 4 &&
        f[20] == 0 && (f[21] == 1 || f[21] == 2)) {
        ip = f + 28;
    } else if (eth == 0x0800 && n >= 34 && (f[14] >> 4) == 4) {
        const size_t header = (f[14] & 15) * 4u;
        const size_t total = ((unsigned)f[16] << 8) | f[17];
        if (header >= 20 && total >= header && n >= 14 + total) ip = f + 26;
    } else if (eth == 0x86dd && n >= 54 && (f[14] >> 4) == 6 &&
               n >= 54u + (((unsigned)f[18] << 8) | f[19])) {
        /* Preserve the donor's global-unicast-only observation policy. */
        if ((f[22] & 0xe0) == 0x20) {
            memcpy(s->ipv6, f + 22, 16);
            s->valid6 = true;
        }
    }
    if (ip && (ip[0] | ip[1] | ip[2] | ip[3])) {
        memcpy(s->ipv4, ip, 4);
        s->valid4 = true;
    }
}
