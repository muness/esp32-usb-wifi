#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p .cache/host-tests
read -r -a compiler <<< "${CC:-cc}"
"${compiler[@]}" -std=c11 -Wall -Wextra -Werror ${TEST_CFLAGS:-} -I main main/host_observation.c tests/host_observation_test.c -o .cache/host-tests/observation
.cache/host-tests/observation
"${compiler[@]}" -std=c11 -Wall -Wextra -Werror ${TEST_CFLAGS:-} -pthread -I tests/stubs -I main tests/bridge_stats_test.c -o .cache/host-tests/stats
.cache/host-tests/stats
