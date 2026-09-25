// SPDX-License-Identifier: MIT
#include "core.h"
#include "view.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static void packets(void) {
    addresses_t a = {0};
    uint8_t f[1600] = {0}, mac[6] = {1, 2, 3, 4, 5, 6};
    for (size_t n = 0; n < 14; n++) {
        observe_packet(&a, f, n, 1);
        assert(!a.valid4 && !a.valid6);
        assert(!frame_reflected(f, n, mac));
    }
    memcpy(f + 6, mac, 6);
    assert(frame_reflected(f, 14, mac));
    f[6] ^= 1;
    assert(!frame_reflected(f, 14, mac));
    f[12] = 8;
    f[13] = 0;
    f[14] = 0x45;
    f[17] = 20;
    f[26] = 192;
    f[27] = 168;
    f[28] = 1;
    f[29] = 8;
    for (size_t n = 14; n < 34; n++) {
        observe_packet(&a, f, n, 10);
        assert(!a.valid4);
    }
    observe_packet(&a, f, 34, 10);
    assert(a.valid4 && a.ip4[3] == 8);
    assert(address_fresh(a.valid4, a.seen4, 60009));
    assert(!address_fresh(a.valid4, a.seen4, 60010));
    assert(!address_fresh(true, 10, 9));
    a = (addresses_t){0};
    f[14] = 0x44;
    observe_packet(&a, f, 34, 11);
    assert(!a.valid4);
    f[14] = 0x45;
    f[17] = 19;
    observe_packet(&a, f, 34, 11);
    assert(!a.valid4);
    memset(f, 0, sizeof(f));
    f[12] = 8;
    f[13] = 6;
    f[15] = 1;
    f[16] = 8;
    f[18] = 6;
    f[19] = 4;
    f[21] = 1;
    f[28] = 10;
    f[31] = 2;
    observe_packet(&a, f, 41, 22);
    assert(!a.valid4);
    observe_packet(&a, f, 42, 22);
    assert(a.valid4 && a.ip4[3] == 2);
    a = (addresses_t){0};
    f[18] = 5;
    observe_packet(&a, f, 42, 22);
    assert(!a.valid4);
    memset(f, 0, sizeof(f));
    f[12] = 0x86;
    f[13] = 0xdd;
    f[14] = 0x60;
    f[22] = 0xfe;
    f[23] = 0x80;
    for (size_t n = 14; n < 54; n++) {
        observe_packet(&a, f, n, 22);
        assert(!a.valid6);
    }
    observe_packet(&a, f, 54, 22);
    assert(a.valid6 && a.ip6[0] == 0xfe);
    a = (addresses_t){0};
    f[22] = 255;
    observe_packet(&a, f, 54, 22);
    assert(!a.valid6);
    f[22] = 0xfe;
    f[19] = 1;
    observe_packet(&a, f, 54, 22);
    assert(!a.valid6);
    /* Deterministic arbitrary short/malformed corpus under sanitizers. */
    uint32_t rng = 1;
    for (int k = 0; k < 100000; k++) {
        for (unsigned j = 0; j < sizeof(f); j++) {
            rng = rng * 1664525 + 1013904223;
            f[j] = rng >> 24;
        }
        observe_packet(&a, f, k % sizeof(f), k);
    }
}
static void rates(void) {
    rates_t r = {0};
    rate_update(&r, UINT64_C(1) << 40, 0, 1000000);
    assert(r.down_mbps == 0);
    rate_update(&r, (UINT64_C(1) << 40) + 1000000, 500000, 2000000);
    assert(fabs(r.down_mbps - 8) < 1e-9 && fabs(r.up_mbps - 4) < 1e-9);
    rate_update(&r, r.down, r.up, 2000000);
    assert(r.down_mbps == 0);
    rate_update(&r, 1, 1, 3000000);
    assert(r.down_mbps == 0);
    for (unsigned i = 0; i < 100; i++)
        rate_update(&r, 1 + i, 1 + i, 4000000 + i * 250000);
    assert(isfinite(r.down_mbps));
}
static void profiles(void) {
    settings_t s = {.version = 1, .brightness = 60, .dim_seconds = 60};
    assert(settings_valid(&s));
    assert(policy_next(&s, 0) == -1);
    s.p[0] = (profile_t){.name = "Home", .ssid = "test", .pass = "12345678", .priority = 20};
    s.p[1] = (profile_t){.name = "Phone", .ssid = "hotspot", .priority = 99};
    s.p[2] = s.p[0];
    s.p[2].priority = 50;
    assert(policy_next(&s, 0) == 0);
    assert(policy_next(&s, 1) == 1);
    assert(policy_next(&s, 3) == 2);
    assert(policy_next(&s, 7) == -1);
    s.preferred = 2;
    assert(policy_next(&s, 0) == 2);
    assert(profile_valid(&s.p[0]));
    strcpy(s.p[0].pass, "short");
    assert(!profile_valid(&s.p[0]));
    memset(s.p[0].ssid, 'x', 33);
    assert(!profile_valid(&s.p[0]));
    s.preferred = 8;
    assert(!settings_valid(&s));
    assert(retry_delay_ms(0) == 1000);
    assert(retry_delay_ms(5) == 30000);
    assert(retry_delay_ms(1000) == 30000);
}
static void buttons(void) {
    button_t b = {0};
    assert(button_update(&b, true, false, 10) == BUTTON_NONE);
    assert(button_update(&b, false, false, 20) == BUTTON_NONE);
    assert(button_update(&b, true, false, 25) == BUTTON_NONE);
    assert(button_update(&b, true, false, 55) == BUTTON_NONE);
    assert(button_update(&b, false, false, 200) == BUTTON_NONE);
    assert(button_update(&b, false, false, 230) == BUTTON_SHORT);
    assert(button_update(&b, true, false, 1000) == BUTTON_NONE);
    button_update(&b, true, false, 1030);
    assert(button_update(&b, true, false, 2530) == BUTTON_HOLD);
    assert(button_update(&b, true, false, 5000) == BUTTON_NONE);
    button_update(&b, false, false, 5010);
    assert(button_update(&b, false, false, 5040) == BUTTON_NONE);
    button_update(&b, true, true, 6000);
    assert(button_update(&b, true, true, 6030) == BUTTON_WAKE);
    assert(button_update(&b, true, false, 9000) == BUTTON_NONE);
    button_update(&b, false, false, 9010);
    assert(button_update(&b, false, false, 9040) == BUTTON_NONE);
}
static void layout(void) {
    view_t v = {.rssi = -128,
                .down_mbps = 123456.78,
                .up_mbps = 98765.43,
                .up_bytes = UINT64_MAX,
                .down_bytes = UINT64_MAX,
                .heap = UINT32_MAX,
                .min_heap = UINT32_MAX};
    memset(v.ssid, 'W', 32);
    memset(v.error, 'X', 47);
    strcpy(v.ip, "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
    v.ip_valid = true;
    for (unsigned p = 0; p < 4; p++)
        for (unsigned d = 0; d < 3; d++) {
            v.page = p;
            v.detail = d;
            char lines[5][32];
            view_lines(&v, lines);
            for (int i = 0; i < 5; i++)
                assert(strlen(lines[i]) <= 26);
            if (p == 0)
                assert(!strcmp(lines[4], "Internet: not checked"));
        }
    v.page = 0;
    v.associated = false;
    strcpy(v.ssid, "Offline");
    char offline[5][32];
    view_lines(&v, offline);
    assert(strstr(offline[0], "RSSI --"));
    char out[5];
    text_clip(out, sizeof(out), "abcdef", 26);
    assert(!strcmp(out, "a..."));
}
int main(void) {
    packets();
    rates();
    profiles();
    assert(trial_decision(44000, 0, 35000, true) == 0);
    assert(trial_decision(45000, 0, 35000, true) == 1);
    assert(trial_decision(45000, 0, 35001, true) == -1);
    assert(trial_decision(46000, 0, 36000, true) == -1);
    assert(trial_decision(45000, 0, 0, false) == -1);
    buttons();
    layout();
    puts("PASS: packet parsing/fuzz, freshness, rates, profiles/backoff, button "
         "gestures, layout");
}
