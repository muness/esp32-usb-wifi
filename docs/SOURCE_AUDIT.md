# Source and hardware audit — 2026-09-25

This is a **source audit**, not identification of a physically attached dongle or an installed TeslaAndroid image. Exact revisions are also machine-readable in [upstreams.json](upstreams.json). Source was fetched from GitHub and inspected locally; no upstream code was executed on hardware.

| Source | Exact commit | Inspection / disposition |
|---|---|---|
| [DrWhax networking donor](https://github.com/DrWhax/esp32-usb-wifi/tree/7384f7290cf4e567b2316e8aa23e95262c7e4999) | `7384f7290cf4e567b2316e8aa23e95262c7e4999` | Read `main/tusb_ncm_main.c`, `bridge.h`, `console.c`, `led.c`, manifests, both Python tools. Reused L2 forwarding design, reflection filter, RTC crash structure and console/provisioning ideas. Removed WS2812 driver entirely. Replaced callback-time command processing/NVS, stale address handling, fixed retry and credential store. |
| [LILYGO original board](https://github.com/Xinyuan-LilyGO/T-Dongle-S3/tree/bb7654607d280fc2a1451d24abf6ed027287d416) | `bb7654607d280fc2a1451d24abf6ed027287d416` | Read original-board docs, `schematic/T-Dongle-S3.pdf`, `T-Dongle-S3-QWIIC.pdf`, `examples/lvgl9/`, factory screen driver/example and font headers. Copied two ST7735 driver files; adapted display setup. No Arduino, factory fonts/artwork, SD, Dual or Plus setup imported. |
| [ESP-IDF v5.5.1](https://github.com/espressif/esp-idf/tree/fcae32885b0296b32044cb99ecbdc50d98dddb83/examples/peripherals/usb/device/tusb_ncm) | `fcae32885b0296b32044cb99ecbdc50d98dddb83` | Requested NCM example path exists at this pinned version. It uses direct station transmit/receive and TinyUSB, without a station netif. Toolchain: xtensa-esp-elf GCC 14.2.0, esp-14.2.0_20241119. |
| [Espressif TinyUSB integration 2.0.1](https://github.com/espressif/esp-usb/tree/6757c6ea4fff779eae8ecb30df5442544ee0fe9b/device/esp_tinyusb) | `6757c6ea4fff779eae8ecb30df5442544ee0fe9b` | Vendored under `components/esp_tinyusb`; original registry hash `a8558cc89dae68552cb7a20123ffcb8db81a8a25bd560bc4d5b2f60a12bd6afb`. Patched sync-send timeout ownership and configuration power descriptor. See THIRD_PARTY. |
| [TinyUSB ESP component 0.21.0~2](https://github.com/espressif/tinyusb/tree/894ea01409e0407a7dbe0ee29b2d58a4691f9046) | `894ea01409e0407a7dbe0ee29b2d58a4691f9046` | Exact version in manifest and hash in lock. Inspected NCM link notifications and synchronous copying in `tud_network_xmit`. Uses `tud_network_default_link_state_cb` and `tud_network_link_state`. |
| [LVGL 9.3.0](https://github.com/lvgl/lvgl/tree/c033a98afddd65aaafeebea625382a94020fe4a7) | `c033a98afddd65aaafeebea625382a94020fe4a7` | Version and package hash locked. Montserrat 10, with its separate OFL and embedded FontAwesome license retained. SPI source/DMA lifetimes do not use LILYGO's immediate completion pattern. |
| [esp-iot-bridge wireless_nic](https://github.com/espressif/esp-iot-bridge/tree/667766b0feefb199afc5de7c59fc330048641fcb/examples/wireless_nic) | `667766b0feefb199afc5de7c59fc330048641fcb` | README and implementation inspected. NAT and ECM/RNDIS available there. Not reused: no demonstrated need for routing or a second USB network class. Example source Apache-2.0. |
| [TeslaAndroid device tree](https://github.com/tesla-android/android-raspberry-pi/tree/7104721dd81f4a4e2d639d0cd7a10ae075ca33bd) | `7104721dd81f4a4e2d639d0cd7a10ae075ca33bd` | README, TeslaAndroid manifest and RNDIS patches inspected. Historical RNDIS patch does not establish the installed kernel's config. Audit only, no source incorporated. |
| [TeslaAndroid Broadcom kernel](https://github.com/tesla-android/android-kernel-broadcom/tree/bdaf9eb15e9ed2cd8c59ba3dc4031d686e2bb6ff) | `bdaf9eb15e9ed2cd8c59ba3dc4031d686e2bb6ff` | `arch/arm64/configs/lineageos_rpi4_defconfig`: USB_USBNET, CDCETHER and CDC_NCM are `y`; RNDIS_HOST unset. `drivers/net/usb/cdc_ncm.c` present. That file offers GPL-2.0 or BSD-2-Clause; kernel overall GPL-2.0. Audit only. |

TeslaAndroid's pinned USB initialiser `2f161ece9e52f8f01e12fa27f7d37f13d509f802` in `android-external-tesla-android-usb-networking-initialiser` was inspected too: it issues special commands for known cellular/Apple devices. No custom adapter command or companion service is added here. This source evidence is encouraging for NCM, **not proof** of Ethernet framework acceptance, DHCP, DNS or downstream-client routing in the user's installed release.

## Board findings

Original-board documentation: ESP32-S3, 16 MB quad flash, 512 KB SRAM, **no PSRAM**, ST7735 160×80. The expected map is centralized in `main/board.h`:

| Function | GPIO |
|---|---|
| USB D− / D+ | 19 / 20 |
| LCD MOSI / CLK / CS / DC / RESET | 3 / 5 / 4 / 2 / 1 |
| Backlight, active low | 38 |
| BOOT button, active low | 0 |
| APA102 data / clock | 40 / 39 |

The original schematic is dated 2022-10-08. Its ESP32-S3 GPIO19/20 nets connect USB_DN/DP; LCD/LED nets match the board examples. Both original and QWIIC variants preserve this interface map. **The schematic's U3 flash label is W25Q32, inconsistent with the product's 16 MB table.** Firmware targets the documented 16 MB original board, but physical silkscreen/revision, `esptool chip_id`/`flash_id`, and flash size must be checked before flashing. Do not infer the user's actual revision from a repository name.

Display setup follows `lvgl9.ino`: BGR, RGB565, inversion enabled, swap XY, gap `(1,26)`, mirror `(false,true)`. This firmware uses 20 MHz SPI initially (example uses 40 MHz), 16-row partial buffers and configurable opposite landscape orientation. Alignment, color order and readability remain physical checks. The GPIO38 WS2812 collision is avoided by not compiling any donor LED code.

Published input: **4.8–5.5 V; 800 mA maximum**. Schematics show USB VBUS supplying the board regulator/backlight; there is no extra supply in this design. USB2 descriptor requests 500 mA, the standard high-power budget, and disables remote wakeup. It does not promise the board stays within that budget: measure current peaks and rail droop on the Pi with Wi-Fi+LCD active. USB suspend power compliance is also pending; this development build does not implement deep USB suspend power reduction. Brownouts or over-budget current are release blockers, not reasons to claim tested bus-power reliability.

## License scope

Do not treat one repository license as covering all binary dependencies. The exact reused-file review and retained texts are in [THIRD_PARTY.md](THIRD_PARTY.md). No custom factory fonts, logos or images were copied. IDF's Wi-Fi/PHY blobs retain Espressif's binary licenses; package generation retains the relevant dependency license material.
