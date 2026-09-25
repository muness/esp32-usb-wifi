// SPDX-License-Identifier: MIT
#include "view.h"
#include <stdio.h>
#include <string.h>
int main(void) {
    view_t v = {.rssi = -58,
                .associated = true,
                .usb_ready = true,
                .usb_mounted = true,
                .ip_valid = true,
                .down_mbps = 3.8,
                .up_mbps = .4,
                .age_ms = 2500,
                .heap = 93240,
                .min_heap = 79120,
                .uptime = 3510,
                .connection_uptime = 3400,
                .active = 0};
    strcpy(v.ssid, "SYNTHETIC-HOME-WIFI-LONG-NAME");
    strcpy(v.name, "Home (synthetic)");
    strcpy(v.ip, "192.0.2.84");
    strcpy(v.ap_ssid, "TDongle-EXAMPLE");
    strcpy(v.ap_pass, "example-only-key");
    strcpy(v.error, "AP not found (synthetic)");
    for (int p = 0; p < 5; p++) {
        char lines[5][32];
        v.page = p % 4;
        v.setup = p == 4;
        view_lines(&v, lines);
        printf("PAGE %d\n", p);
        for (int i = 0; i < 5; i++)
            puts(lines[i]);
    }
}
