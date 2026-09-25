#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Read-only host evidence; does not reconfigure interfaces or require root.
With --adb, uses an already-authorized adb connection. No root, install, reboot,
setprop, DHCP command, or network mutation. Output contains network identifiers.
"""
import argparse
import json
import platform
import subprocess


def capture(argv):
    try:
        r = subprocess.run(argv, capture_output=True, text=True, timeout=12)
        return {'command': argv, 'exit': r.returncode, 'stdout': r.stdout[:65536], 'stderr': r.stderr[:4096]}
    except (OSError, subprocess.TimeoutExpired) as e:
        return {'command': argv, 'unavailable': type(e).__name__}


def commands(adb=False, system=None):
    if adb:
        return [['adb', 'shell', *cmd] for cmd in [
            ['getprop', 'ro.build.fingerprint'], ['getprop', 'ro.build.version.release'],
            ['uname', '-a'], ['ip', '-details', 'link'], ['ip', 'address'],
            ['ip', 'route', 'show', 'table', 'all'], ['ip', '-6', 'route'],
            ['dumpsys', 'ethernet'], ['dumpsys', 'connectivity'], ['dumpsys', 'usb'],
            ['cat', '/proc/modules'],
            ['sh', '-c', 'zcat /proc/config.gz 2>/dev/null | grep -E "CONFIG_USB_(USBNET|NET_CDC_NCM|NET_CDCETHER|NET_RNDIS_HOST)="'],
        ]]
    if (system or platform.system()) == 'Darwin':
        return [['sw_vers'], ['system_profiler', 'SPUSBDataType'], ['ifconfig'], ['netstat', '-rn'], ['scutil', '--dns']]
    return [['uname', '-a'], ['lsusb', '-t'], ['lsusb'], ['ip', '-details', 'link'], ['ip', 'address'], ['ip', 'route', 'show', 'table', 'all'], ['ip', '-6', 'route'], ['resolvectl', 'status']]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--adb', action='store_true')
    args = p.parse_args()
    print(json.dumps({'scope': 'read-only; unavailable tools/permissions are recorded',
                      'evidence': [capture(c) for c in commands(args.adb)]}, indent=2))


if __name__ == '__main__':
    main()
