# Physical acceptance and measurement record

**All physical tests below are pending.** No ESP32, Pi 400, macOS host or TeslaAndroid device was accessed in this implementation session. Checkboxes are deliberately unchecked. Keep the existing PIX-LINK available for rollback. Do not change TeslaAndroid routing or flash any device without owner authorization.

## Record before testing

Firmware version, artifact SHA256, board silkscreen/revision and photos, ROM chip/flash identity, flash frequency/mode, host model/OS, TeslaAndroid release/build fingerprint/kernel, USB topology/other loads, AP model/firmware/auth/channel/subnet, signal strength/distance, Pi supply model and measured rail/current. Preserve read-only host evidence before/after. Repository driver presence alone is not installed-image evidence.

## 1. Board/power and headless baseline

- [ ] Confirm original board, 16 MB flash (schematic discrepancy), no PSRAM; only then authorize first flash.
- [ ] Check 5 V power and peak/steady current on Pi port at boot, AP mode, association, full traffic and dim/bright UI. Confirm USB2 500 mA budget or stop and resolve; published board maximum is 800 mA.
- [ ] Check LCD offsets/color/inversion/rotation, backlight active-low levels, BOOT ROM entry, APA102 fixed pins. No SD required.
- [ ] Inspect descriptors on Linux (`lsusb -v -d 303a:` if available) and macOS System Information: NCM control+data, ACM optional, stable eFuse serial/MAC, bus-powered 500 mA. Record actual VID/PID from artifact descriptor check.
- [ ] Linux and macOS enumerate repeatedly; NCM driver loads, host uses advertised MAC, Wi-Fi association produces carrier; USB host-interface readiness remains a separate observation.
- [ ] Pi obtains a real upstream lease; verify router lease record / host DHCP evidence independently of passive adapter IP. ARP, static IPv4, DNS and bidirectional traffic work. Clear source observation on link change; expire after 60 s silence.
- [ ] IPv6 SLAAC/DAD, link-local/ULA/global address, ND, multicast/mDNS work where upstream supports them; reflection filter does not cause duplicate-address failures. VLANs are intentionally unsupported.

## 2. Actual TeslaAndroid coexistence

- [ ] Record installed build fingerprint and kernel; run read-only diagnostics (adb only if already available/authorized). Missing tools/permissions remain missing evidence, not grounds to request root.
- [ ] Plug adapter into Pi; observe interface/driver/carrier, upstream address, routes and DNS. No custom app/kernel/daemon or manual route change.
- [ ] Existing Pi Wi-Fi AP remains available; existing Tesla display/browser continues functioning simultaneously.
- [ ] Generate actual Internet traffic from Pi **and from the downstream TeslaAndroid client**; confirm DNS, browsing, streaming and expected upstream route. Check that the routed client traffic uses the Pi NCM MAC.
- [ ] If composite enumeration fails, preserve descriptors/logs then test network-only variant. Only consider ECM/RNDIS or NAT with a documented failing step and evidence; do not silently change host networking.

## 3. Recovery matrix

For each case record time to carrier, host address refresh, restored downstream browser traffic, UI error/freshness, drop counters and memory:

- [ ] AP missing at boot; wrong password; WPA2; WPA3-SAE; transition; open AP.
- [ ] AP power restart; phone hotspot off/on; different subnet; static-host address case.
- [ ] Preferred AP absent and another saved profile available; all profiles absent; preferred returns during a healthy fallback connection (must not oscillate); explicit selection.
- [ ] USB unplug/replug, host reboot, USB bus reset, host suspend/resume. USB suspend current compliance specifically remains a release gate.
- [ ] At least 30 cold starts; first boot, saved config and setup timeout; check warm-reset versus cold-reset diagnostics.
- [ ] Sustained full queues / USB backpressure; no packet-buffer leak or double free. Packet ownership host tests complement, not replace, real stress.
- [ ] Watchdog/panic recovery in a **separate test build** or debugger-triggered fault, with owner approval. Production console has no accidental `hang`/`crash` commands.

## 4. Provisioning and UI

- [ ] Serial interactive password is not echoed/stored in shell history/log export. Status never prints upstream or AP credentials.
- [ ] Add/edit/delete/select all eight slots, named priorities; 32-character SSID; hidden SSID entered manually; open network; invalid/overlength/Unicode/control/escaped-NUL inputs rejected; malformed JSON and duplicate fields rejected.
- [ ] AP portal on mobile Safari/Chrome; no external resources; random password changes on re-entry, token rejects cross-origin/missing-token requests; request size and slow-client timeouts.
- [ ] Cancel and ten-minute timeout restore saved adapter mode; invalid submission reports error without erasing old profile.
- [ ] Power interruption before candidate commit, during trial and during NVS update: old working config survives; schema-corrupt state does not auto-reset.
- [ ] Candidate must achieve ten continuous seconds association within 45 seconds; failure reverts. Successful association alone must never show Internet OK.
- [ ] Short press page cycle; debounce under switch chatter; long hold one-shot; dim wake consumes entire gesture; menu hold selection; reset requires release and second hold; reset timeout/cancel.
- [ ] Worst-length SSID/error/IPv6 via console; no row overflow at 160×80; orientation, dimming and brightness; failed optional display/LED still permits network operation.

## 5. Benchmark and soak

Use an upstream LAN `iperf3` server to separate adapter performance from Internet variability. Use only tools already available on each host or explicitly authorized installations. If Android lacks iperf, measure from a downstream browser/client and state that path; do not invent Pi-only figures.

Example on an authorized Linux/macOS test host:

```sh
iperf3 -c SERVER -t 120 -P 1 --json > upload.json
iperf3 -c SERVER -R -t 120 -P 1 --json > download.json
iperf3 -c SERVER --bidir -t 120 --json > bidirectional.json
ping -c 100 SERVER > idle-latency.txt
# Repeat ping under load, then repeat same conditions with UI headless build.
```

D = upstream Wi-Fi → USB host, U = USB host → upstream Wi-Fi. Run three repetitions of each direction; report median plus range, packet loss, idle/loaded median and p95 latency. Record screen brightness, RSSI, AP channel/interference, NCM host driver and concurrent traffic. Donor ~5.7 Mbps is historical context **not a target result or promise**.

- [ ] Full UI versus headless throughput/latency/power/memory comparison.
- [ ] Current/min heap, DMA heap, control/UI stack watermarks, frame drops/reflections, reconnects, USB resets and reset reason recorded before/after.
- [ ] Overnight (8–12 hours) bidirectional soak plus periodic downstream browsing; time series of counters/heap, no unbounded decline or unexpected reboot.
- [ ] AP-loss/USB-reset during sustained load; connection/UI state clears correctly and restores without reflashing.

| Build / conditions | D Mb/s | U Mb/s | Loaded p95 ms | Min heap | Drops | Resets | Duration |
|---|---:|---:|---:|---:|---:|---:|---|
| No hardware measurements yet | — | — | — | — | — | — | — |
