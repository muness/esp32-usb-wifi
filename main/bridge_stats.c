/* SPDX-License-Identifier: MIT */
#include "freertos/FreeRTOS.h"

#include "bridge.h"

/* One lock protects frame counts, byte totals, drops, and every snapshot. */
static portMUX_TYPE s_stats_lock = portMUX_INITIALIZER_UNLOCKED;
static bridge_stats_t s_stats;

void bridge_count_frame(bridge_direction_t direction, uint16_t frame_len) {
  portENTER_CRITICAL(&s_stats_lock);

  if (direction == BRIDGE_TO_WIFI) {
    /* The Pi sent this frame toward the Wi-Fi access point. */
    s_stats.host_to_wifi++;
    s_stats.upload_bytes += frame_len;
  } else {
    /* Wi-Fi delivered this frame toward the Pi over USB. */
    s_stats.wifi_to_host++;
    s_stats.download_bytes += frame_len;
  }

  portEXIT_CRITICAL(&s_stats_lock);
}

void bridge_count_drop(bridge_drop_t kind) {
  portENTER_CRITICAL(&s_stats_lock);

  switch (kind) {
  case BRIDGE_DROP_TX:
    s_stats.txdrop++;
    break;
  case BRIDGE_DROP_REFLECTED:
    s_stats.reflected++;
    break;
  case BRIDGE_DROP_POOL:
    s_stats.poolfail++;
    break;
  case BRIDGE_DROP_RX:
    s_stats.rxdrop++;
    break;
  }

  portEXIT_CRITICAL(&s_stats_lock);
}

void bridge_get_stats(bridge_stats_t *out) {
  portENTER_CRITICAL(&s_stats_lock);
  *out = s_stats;
  portEXIT_CRITICAL(&s_stats_lock);
}
