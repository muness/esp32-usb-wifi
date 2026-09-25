#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p .cache/host-tests
read -r -a compiler <<< "${CC:-cc}"
"${compiler[@]}" -std=c11 -Wall -Wextra -Werror ${TEST_CFLAGS:-} -I main main/host_observation.c tests/host_observation_test.c -o .cache/host-tests/observation
.cache/host-tests/observation
