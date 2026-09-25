#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Interactive provisioning; no password arguments, command echo or history.
Serial discovery/read-until-quiet adapted from DrWhax/esp32-usb-wifi (MIT).
Explicit port is required to avoid probing unrelated serial devices.
"""
import argparse
import getpass
import json
import time


def validate_profile(p):
    for key, maximum, minimum in [('name', 24, 1), ('ssid', 32, 1), ('password', 63, 0)]:
        value = p[key]
        if not minimum <= len(value) <= maximum or any(not 32 <= ord(c) < 127 for c in value):
            raise ValueError(f'{key}: printable ASCII, {minimum}..{maximum} characters')
    if p['password'] and len(p['password']) < 8:
        raise ValueError('Password: empty for open or 8..63 characters')
    if type(p['slot']) is not int or not 1 <= p['slot'] <= 8:
        raise ValueError('Slot must be 1..8')
    if type(p['priority']) is not int or not 0 <= p['priority'] <= 100:
        raise ValueError('Priority must be 0..100')
    return p


def read_until_quiet(port, timeout=3):
    out = bytearray()
    end = time.monotonic() + timeout
    quiet = end
    while time.monotonic() < min(end, quiet):
        data = port.read(1024)
        if data:
            out.extend(data)
            quiet = time.monotonic() + 0.4
        if len(out) > 16384:
            raise RuntimeError('Unexpectedly long response')
    return out.decode('utf-8', 'replace')


def command(port, line, timeout=3):
    port.write(line.encode('ascii') + b'\n')
    port.flush()
    return read_until_quiet(port, timeout)


def profile_input(port):
    print(command(port, 'scan', 8))
    # SSID is typed after inspecting scan results; hidden networks work too.
    p = {'slot': int(input('Profile slot [1..8]: ')),
         'name': input('Profile name: '), 'ssid': input('SSID (exact): '),
         'priority': int(input('Priority [0..100]: ')),
         'password': getpass.getpass('Password (empty for open): ')}
    validate_profile(p)
    encoded = 'profile ' + json.dumps(p, separators=(',', ':'), ensure_ascii=True)
    if len(encoded) >= 512:
        raise ValueError('Encoded profile exceeds protocol limit')
    # Firmware never echoes commands. Do not print raw replies to a secret-
    # bearing request, even if the user accidentally selected another device.
    command(port, encoded)
    p['password'] = ''
    print('Trial requested. The dongle reboots; check its screen, then reconnect for status.')


def menu(port):
    while True:
        print('\n1 Status/diagnostics  2 Scan  3 Profiles  4 Add/edit profile\n'
              '5 Select profile  6 Delete profile  7 Browser setup\n'
              '8 Display settings  9 Factory reset  0 Quit')
        choice = input('Choice: ').strip()
        if choice == '0':
            return
        if choice == '4':
            profile_input(port)
            return
        if choice in {'1', '2', '3', '7'}:
            print(command(port, {'1': 'status', '2': 'scan', '3': 'list', '7': 'setup'}[choice], 8))
            if choice == '7':
                return
        elif choice in {'5', '6'}:
            slot = int(input('Slot [1..8]: '))
            if not 1 <= slot <= 8:
                raise ValueError('Slot must be 1..8')
            if choice == '6' and input('Type DELETE to delete this profile: ') != 'DELETE':
                continue
            print(command(port, ('use ' if choice == '5' else 'del ') + str(slot)))
        elif choice == '8':
            brightness = int(input('Brightness [5..100]: '))
            rotation = int(input('Orientation [0 normal, 1 upside down]: '))
            seconds = int(input('Dim after seconds [10..3600]: '))
            print(command(port, f'display {brightness} {rotation} {seconds}'))
        elif choice == '9':
            if input('Type ERASE ALL PROFILES to confirm: ') == 'ERASE ALL PROFILES':
                print(command(port, 'reset'))
                command(port, 'confirm-reset')
                print('Reset requested.')
                return


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--port', required=True, help='/dev/ttyACM0 or /dev/cu.usbmodem...')
    p.add_argument('--status', action='store_true', help='Read-only adapter diagnostics')
    p.add_argument('--provision', action='store_true', help='Interactive add/edit profile')
    args = p.parse_args()
    import serial
    with serial.Serial(args.port, 115200, timeout=0.1, write_timeout=2) as port:
        time.sleep(0.3)
        banner = command(port, 'help')
        if 'Profiles validate by association' not in banner:
            raise RuntimeError('Port did not identify this firmware; no configuration sent')
        if args.status:
            print(command(port, 'status', 5))
        elif args.provision:
            profile_input(port)
        else:
            menu(port)


if __name__ == '__main__':
    try:
        main()
    except (ValueError, RuntimeError, OSError, KeyboardInterrupt) as exc:
        raise SystemExit(str(exc))
