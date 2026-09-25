/* Shared interface between the bridge core (tusb_ncm_main.c) and the
 * management console (console.c). */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  /* Direction is relative to the Pi: upload Pi->Wi-Fi, download Wi-Fi->Pi. */
  uint64_t upload_bytes;
  uint64_t download_bytes;

  uint32_t host_to_wifi; /* Pi -> Wi-Fi frames accepted for forwarding. */
  uint32_t wifi_to_host; /* Wi-Fi -> Pi frames accepted for forwarding. */
  uint32_t txdrop;       /* Pi -> Wi-Fi dropped while not associated. */
  uint32_t reflected;    /* AP echo of a frame previously sent by the Pi. */
  uint32_t poolfail;     /* Wi-Fi driver had no transmit buffer. */
  uint32_t rxdrop;       /* USB NCM transmit failed or timed out. */
} bridge_stats_t;

/* Coherent snapshot, including 64-bit fields on 32-bit ESP targets.
 * Byte totals are per boot, not proof of receipt or Internet reachability. */
void bridge_get_stats(bridge_stats_t *s);

/* Forwarding directions are named from the Pi's point of view. */
typedef enum {
  BRIDGE_TO_WIFI, /* Pi -> upstream access point. */
  BRIDGE_TO_HOST, /* Upstream access point -> Pi. */
} bridge_direction_t;

/* Count a frame only after the forwarding path accepts it. */
void bridge_count_frame(bridge_direction_t direction, uint16_t frame_len);

/* Classify drops without logging packet contents from the forwarding path. */
typedef enum {
  BRIDGE_DROP_TX,        /* No associated Wi-Fi station for Pi -> AP. */
  BRIDGE_DROP_REFLECTED, /* AP echoed a frame previously sent by the Pi. */
  BRIDGE_DROP_POOL,      /* Wi-Fi TX driver had no free buffer. */
  BRIDGE_DROP_RX,        /* USB NCM transmit failed or timed out. */
} bridge_drop_t;
void bridge_count_drop(bridge_drop_t kind);
void bridge_get_mac(uint8_t mac[6]);
bool bridge_wifi_connected(void);

/* "up" when associated; otherwise the last failure reason mapped pico-style:
 * "badauth" (wrong passphrase), "nonet" (SSID not found), or "join". */
const char *bridge_link_status(void);

/* Crash telemetry that survives warm reboots (RTC noinit RAM). */
typedef struct {
  uint32_t boots;        /* boots since the last cold power-on */
  uint32_t hangs;        /* watchdog-reset recoveries since cold power-on */
  uint32_t faults;       /* panic recoveries since cold power-on */
  const char *recovered; /* "panic", "watchdog", or NULL. */
  bridge_stats_t pre;    /* counters snapshotted just before the crash */
} bridge_crash_info_t;

void bridge_get_crash(bridge_crash_info_t *c);

/* Regulatory country as saved in the config ("" = driver default). */
const char *cfg_country(void);

/* Host addresses snooped passively from host -> Wi-Fi frames (the bridge holds
 * no IP of its own). Not evidence of DHCP success or Internet access.
 * Observations expire after 60 seconds without a valid source frame and clear
 * on association changes, credential changes, USB reset or unplug. */
bool bridge_host_ipv4(uint8_t ip[4]);
bool bridge_host_ipv6(uint8_t ip[16]);

/* Drop any association and re-join with these credentials (empty ssid: just
 * disassociate). Safe to call from the console (TinyUSB task) context. */
void wifi_apply_creds(const char *ssid, const char *pass);

/* console.c */
void console_init(void);
/* Load NVS credentials or the compile-time defaults. */
void creds_load(char *ssid, size_t ssid_sz, char *pass, size_t pass_sz);
/* Diagnostics line on the console's debug stream; no-op unless debug is on. */
void console_debug_printf(const char *fmt, ...);

/* led.c — status patterns on the devkit's WS2812 (pico LED-state parity). */
void led_init(void);
void led_set_provisioned(bool have_ssid);
bool led_set_gpio(int gpio); /* re-home the WS2812; false if init failed */
