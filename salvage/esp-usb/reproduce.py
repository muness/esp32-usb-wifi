#!/usr/bin/env python3
"""Expected upstream failure followed by patched success. No network or hardware.
The fixture is esp-usb e0a4a9d46cf00de760edbf869dfb4238e91a0dea.
Requires a native C compiler; CC may contain arguments. Set TEST_CFLAGS for ASan/UBSan.
"""
import os, pathlib, shlex, subprocess, tempfile
root=pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory() as tmp:
    tmp=pathlib.Path(tmp)
    target=tmp/'device/esp_tinyusb/tinyusb_net.c';target.parent.mkdir(parents=True)
    target.write_bytes((root/'fixtures/tinyusb_net.upstream.c').read_bytes())
    for patched in [False,True]:
        if patched:
            subprocess.run(['git','apply',str(root/'send-timeout.patch')],cwd=tmp,check=True)
        body='\n'.join(line for line in target.read_text().splitlines() if not line.startswith('#include'))
        source=tmp/'repro.c';source.write_text((root/'net.h').read_text()+'\n'+body+'\n'+(root/'net_cases.c').read_text())
        binary=tmp/'repro'
        subprocess.run(shlex.split(os.environ.get('CC','cc'))+shlex.split(os.environ.get('TEST_CFLAGS',''))+['-std=c11',str(source),'-o',str(binary)],check=True)
        result=subprocess.run([str(binary)],capture_output=True,text=True)
        if patched:
            assert result.returncode==0,result.stderr
            print('Patched: PASS (copy, cancellation, deadline/copy race, backpressure, recovery)')
        else:
            assert result.returncode!=0,'Upstream unexpectedly passed; re-audit rather than submit stale fix'
            assert 'tinyusb_net_send_sync(bytes,100,(void*)123,20)==ESP_OK' in result.stderr,result.stderr
            print('Upstream: expected assertion failure at deadline/copy race reproduced')
