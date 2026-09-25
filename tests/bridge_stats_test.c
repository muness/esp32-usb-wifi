/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>

/* Compile the production stats module into this host test. */
#include "../main/bridge_stats.c"

enum {
  FRAMES_PER_WRITER = 50000,
  TEST_FRAME_SIZE_BYTES = 1200,
  LARGE_FRAME_SIZE_BYTES = 1514,
};

#define LARGE_COUNTER_BASE (UINT64_C(1) << 40)

typedef struct {
  bridge_direction_t direction;
} writer_args_t;

static atomic_uint s_finished_writers;

static const writer_args_t s_upload_writer = {.direction = BRIDGE_TO_WIFI};
static const writer_args_t s_download_writer = {.direction = BRIDGE_TO_HOST};

static void *count_test_frames(void *context) {
  const writer_args_t *args = context;

  for (unsigned frame = 0; frame < FRAMES_PER_WRITER; frame++) {
    bridge_count_frame(args->direction, TEST_FRAME_SIZE_BYTES);
  }

  atomic_fetch_add_explicit(&s_finished_writers, 1, memory_order_release);
  return NULL;
}

static void test_64_bit_byte_accumulation(void) {
  bridge_stats_t snapshot;

  /* Verify production accounting preserves the high bits of a 64-bit total. */
  s_stats = (bridge_stats_t){.upload_bytes = LARGE_COUNTER_BASE};
  bridge_count_frame(BRIDGE_TO_WIFI, LARGE_FRAME_SIZE_BYTES);
  bridge_get_stats(&snapshot);

  assert(snapshot.upload_bytes == LARGE_COUNTER_BASE + LARGE_FRAME_SIZE_BYTES);
}

static void test_coherent_concurrent_snapshots(void) {
  bridge_stats_t snapshot;
  pthread_t upload_thread;
  pthread_t download_thread;

  /* The test changes module state only before its writer threads start. */
  s_stats = (bridge_stats_t){0};
  atomic_store_explicit(&s_finished_writers, 0, memory_order_relaxed);

  assert(pthread_create(&upload_thread, NULL, count_test_frames,
                        (void *)&s_upload_writer) == 0);
  assert(pthread_create(&download_thread, NULL, count_test_frames,
                        (void *)&s_download_writer) == 0);

  while (atomic_load_explicit(&s_finished_writers, memory_order_acquire) < 2) {
    bridge_get_stats(&snapshot);

    /* Each frame and its bytes must appear together in one locked snapshot. */
    assert(snapshot.upload_bytes ==
           (uint64_t)snapshot.host_to_wifi * TEST_FRAME_SIZE_BYTES);
    assert(snapshot.download_bytes ==
           (uint64_t)snapshot.wifi_to_host * TEST_FRAME_SIZE_BYTES);
  }

  assert(pthread_join(upload_thread, NULL) == 0);
  assert(pthread_join(download_thread, NULL) == 0);
  bridge_get_stats(&snapshot);

  assert(snapshot.host_to_wifi == FRAMES_PER_WRITER);
  assert(snapshot.wifi_to_host == FRAMES_PER_WRITER);
}

static void test_named_drop_counters(void) {
  static const bridge_drop_t all_drop_causes[] = {
      BRIDGE_DROP_TX,
      BRIDGE_DROP_REFLECTED,
      BRIDGE_DROP_POOL,
      BRIDGE_DROP_RX,
  };
  bridge_stats_t snapshot;

  for (size_t cause = 0;
       cause < sizeof(all_drop_causes) / sizeof(all_drop_causes[0]); cause++) {
    bridge_count_drop(all_drop_causes[cause]);
  }

  bridge_get_stats(&snapshot);
  assert(snapshot.txdrop == 1);
  assert(snapshot.reflected == 1);
  assert(snapshot.poolfail == 1);
  assert(snapshot.rxdrop == 1);
}

int main(void) {
  test_64_bit_byte_accumulation();
  test_coherent_concurrent_snapshots();
  test_named_drop_counters();

  puts("PASS: 64-bit byte totals, coherent concurrent snapshots, and named "
       "drop causes");
}
