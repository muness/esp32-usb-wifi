#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
import os, shlex, subprocess
from pathlib import Path
idf=os.environ.get('IDF_PATH')
if not idf:
    print('SKIP: cJSON parser test requires pinned IDF_PATH (firmware CI runs it)')
else:
    source=Path(idf)/'components/json/cJSON'
    subprocess.run(shlex.split(os.environ.get('CC','cc'))+shlex.split(os.environ.get('TEST_CFLAGS',''))+['-std=c11','-I','main','-I',str(source),'main/core.c','main/profile_json.c',str(source/'cJSON.c'),'tests/test_profile_json.c','-lm','-o','build-host/test_profile_json'],check=True)
    subprocess.run(['build-host/test_profile_json'],check=True)
