# Salvage: upstream candidates

Salvaged 2026-09-25. No upstream PR has been opened or authorized.

## Reason and original aim

Replace the external Wi-Fi/Ethernet bridge on a working Pi 400 TeslaAndroid installation with a T-Dongle-S3 USB NCM adapter, including useful local status and provisioning. The independent application achieved software builds and host tests, but its combined board, portal, UI and networking changes obscured which improvements belong upstream. Salvage the small reusable changes while preserving the application and its history.

Formal fork: https://github.com/muness/esp32-usb-wifi (parent DrWhax/esp32-usb-wifi).
Upstream baseline: `7384f7290cf4e567b2316e8aa23e95262c7e4999`.

## Learnings and frame shifts

1. The donor already supplies NCM forwarding, a console, eight saved profiles and diagnostics. Profile switching is not a wholly new network feature; physical controls and the screen are product integrations.
2. Observed source addresses are evidence of traffic, not DHCP success or Internet reachability. Reset boundaries and freshness belong in the observation API, independently of a renderer.
3. A stats UI needs coherent byte totals. Packet counts and Wi-Fi link speed cannot substitute for throughput. Keep byte accounting upstream and rate calculation/rendering in the application.
4. The synchronous TinyUSB send deadline can race with successful copying/freeing. Current esp-usb still reproduces the failure under a deterministic mocked scheduler. This belongs to Espressif, not a hidden board-specific workaround.
5. LILYGO's LVGL example releases a display buffer before asynchronous SPI completion. The isolated callback patch belongs in its example; the full application's defensive display setup is a separate concern.
6. Accepting a sanitizer compiler flag is insufficient evidence of instrumentation: the earlier Zig environment did so without usable ASan instrumentation. These extracted tests use native GCC 13.3 with ASan and UBSan.
7. Concurrent first-time ESP-IDF component downloads shared a mutable cache and caused two configure failures. Serialize initial resolution; retain resolved dependency hashes rather than interpreting this as a source failure.

The frame changes from “submit the dongle application upstream” to “preserve the product, submit individually justified improvements to each owner.” Compilation is not evidence of real USB host compatibility.

## Donor stack

Each branch is one commit above the previous branch. Review each against its immediate parent, not all against main.

| Branch | Commit | Parent | Candidate |
|---|---|---|---|
| `stack/01-observation-validation` | `6df37c5` | `main` | Validate complete ARP/IPv4/IPv6 headers before learning addresses; portable parser and malformed-packet tests |
| `stack/02-observation-lifecycle` | `4cabb63` | stack/01 | Expire observations after 60 seconds; clear on Wi-Fi/USB/network transitions; reject stale in-flight observations using epochs |
| `stack/03-coherent-byte-counters` | `b2547bb` | stack/02 | Locked snapshots of frame/drop/64-bit byte counters and console output; concurrent host tests |

The parser names the Ethernet/IP/ARP field offsets and protocol values, and comments explain the length and version checks; its tests use named frame fields and readable fixture helpers. The branch stack has been restacked after that readability pass. CI will rerun the host tests and all firmware variants against the updated commit. The 60-second observation lifetime is an explicit policy choice for maintainer review. The parser preserves the donor's global IPv6 2000::/3 scope; it does not claim new link-local/multicast forwarding capabilities. Static hosts can populate observations without DHCP. Byte counters are per boot; the existing warm-reset RTC record format remains unchanged. Counters measure successful send acceptance, not confirmed delivery at the remote endpoint.

Hardware checks remain necessary for reset/disconnect races, USB callback context and sustained traffic. The observation epoch does not purport to repair every pre-existing Wi-Fi lifecycle concurrency issue.

## Other owners

* `esp-usb/send-timeout.patch`: Espressif esp-usb at `e0a4a9d46cf00de760edbf869dfb4238e91a0dea`. Apache-2.0 fixture and license retained. Run `python3 salvage/esp-usb/reproduce.py`; it requires the original assertion failure before checking patched success. The scheduler and USB functions are mocked. Actual USB/FreeRTOS validation and upstream test integration remain pending.
* `lilygo/lvgl9-dma-completion.patch`: Xinyuan-LilyGO/T-Dongle-S3 at `bb7654607d280fc2a1451d24abf6ed027287d416`. MIT license retained. `git apply --check` passed against that checkout. Arduino compilation, callback/ISR integration and physical display validation remain pending. It is a candidate patch, not a validated release.

These are patch bundles, not branches pretending to have the donor as their upstream. Create forks of the owning repositories when preparing their submissions. No owner has been contacted.

## Guardrails and missing context

* Keep fork main identical to upstream. Do not combine board support, portal security/NVS policy, dependency upgrades and packet-path fixes into one PR.
* Preserve donor IPv6 observation policy while fixing parsing; broader policy changes need separate justification.
* Do not transplant the donor WS2812 GPIO setup onto the T-Dongle APA102/backlight wiring.
* Firmware built from the donor stack uses donor defaults (2 MB flash), not the T-Dongle product configuration. Do not flash those validation images onto the board as a release.
* No attached hardware was flashed, and no TeslaAndroid settings were changed. The installed TeslaAndroid kernel, actual board flash/power characteristics, USB enumeration and downstream client routing still need physical verification.
* Keep saved upstream secrets out of reports and setup QR codes. Plain NVS is not encrypted storage.

Earlier inspection of exact dependency ownership and upstream diffs would have made extraction cheaper. Maintainer appetite for a 60-second observation policy and public counter APIs is unknown. No upstream acceptance is implied.

## Preserved application and deferred work

`tdongle/integration` now descends from stack/03 and preserves the exact application tree from `2634731ff64b1a96683da4c362960f6bde5cc024` as its second parent. This gives the buildable product branch reviewed fork ancestry while preserving the application history; it does not imply the product still uses the donor modules verbatim. The product bridge was refactored into `bridge.c`/`core.c`: its source includes equivalent coherent byte snapshots and expiring observations, plus product-specific bounded forwarding. The product parser accepts a broader IPv6 source range than donor stack/01, so the candidate parser commit is not identical code to the product parser. Original working source remains in the sibling `t-dongle-s3-adapter` checkout. Its latest firmware-code commit is `61eda53`; the later commit changes documentation. The fork integration tree is byte-for-byte the same Git tree as application commit `2634731` (verified with `git diff --exit-code`).

Keep the ST7735/LVGL UI, button gestures, APA102, browser setup mode, board partitions and provisioning tools on the product branch. Defer NCM notification/dependency upgrades, console worker restructuring and a forwarding pool until their dependencies and behavior can be reviewed independently. Preserve the existing app's bounded buffers and diagnostics as reference material, not as an obligation to upstream its architecture.

The original application records three compiled variants (full, headless, network-only), four native C test executables, eight Python tests, and a byte-identical clean rebuild in its validation documentation. Those results are distinct from this extraction's results; no physical board or TeslaAndroid test has been performed.

## Fresh start kit

1. Read this report and `results/validation.md`. Fetch upstream and inspect changes since the pinned baseline.
2. Review each donor commit against its parent. Rebase the three branches in order if the baseline moves; rerun host tests and an ESP32-S3 build at every boundary.
3. For host tests: `TEST_CFLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" bash tests/run.sh` with native GCC available. For firmware: activate ESP-IDF v5.5.1, use the preserved `results/donor-dependencies.lock` as root `dependencies.lock`, then `idf.py -D IDF_TARGET=esp32s3 build`. The donor manifest remains unchanged; the lock records this validation's exact resolution.
4. Exercise malformed traffic, AP and USB resets, observation expiry, concurrent forwarding and counters on hardware before proposing merge. Use the product branch's acceptance checklist for TeslaAndroid.
5. Ask the user before opening any upstream PR. When authorized, propose the parser first, then lifecycle policy, then accounting; preserve the stated dependency order. Submit external patches only to their respective owners after their outstanding validation.

No host companion is required by these changes. No NAT conversion is proposed.
