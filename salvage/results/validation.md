# Extraction validation — 2026-09-25

ESP-IDF v5.5.1 (`fcae32885b0296b32044cb99ecbdc50d98dddb83`); ESP32-S3; donor default configuration. Resolved registry versions and integrity hashes are preserved in `donor-dependencies.lock`: esp_tinyusb 2.3.0, TinyUSB 0.21.0~2, led_strip 3.0.3.

| Revision | Build | App binary bytes | SHA-256 |
|---|---|---:|---|
| 7384f72 | PASS | 745088 | `028332b5419a116e5e4e61a698adb483749993e7dd07baafe23bab4407f7b038` |
| 1c67f99 | PASS | 745168 | `4a1afeda0572d0638bffbddda5e3cc25f8cefbe76ac9d298035dae1cbf8ea87b` |
| 20a108a | PASS | 745632 | `15a6224b57baf0e59f9f6fee87cc99fe34a854021a5178754a8bafa830186e06` |
| 22a15ad | PASS | 745776 | `a696cb36f380170f08aa3de06e447a92926f40e76a61a33eb7eaedad0b5d7a88` |

All three candidate branches passed their native GCC 13.3 host tests with `-fsanitize=address,undefined -fno-omit-frame-pointer`. Branch 01 covers parser validation; 02 adds timestamp/expiry/reset behavior; 03 additionally runs concurrent counter/snapshot tests. See per-branch test logs.

Current upstream esp-usb failed the deterministic deadline/copy regression at the expected assertion; patched source passed success, cancellation, deadline race, backpressure and recovery cases with native ASan/UBSan. See the reproducer and its log.

The LILYGO patch passed `git apply --check` against its pinned source. It has NOT been compiled or tested on a display.

Initial parallel builds of baseline and branch 01 failed during shared component-cache extraction. A first retry detected incomplete integrity metadata. The incomplete generated managed_components directories were moved aside; sequential dependency resolution and builds then succeeded. No source change was needed.

No hardware tests, flashing, TeslaAndroid changes, physical throughput measurements or upstream PRs were performed. These donor binaries are build evidence only, not T-Dongle release artifacts. Integration commit `34b239d` has the exact tree of application `2634731`, verified by an empty Git diff; the preserved application was not rebuilt for this history-only merge.
