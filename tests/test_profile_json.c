// SPDX-License-Identifier: MIT
#include "core.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    profile_t p;
    int slot;
    const char *valid = "{\"slot\":8,\"priority\":100,\"name\":\"Home\",\"ssid\":"
                        "\"Network\",\"password\":\"12345678\"}";
    assert(profile_parse_json(valid, &slot, &p) && slot == 7 && !strcmp(p.pass, "12345678"));
    const char *bad[] = {"",
                         "{}",
                         "[]",
                         "null",
                         "{",
                         "{\"slot\":1}",
                         "{\"slot\":1,\"slot\":2,\"priority\":1,\"name\":\"x\","
                         "\"ssid\":\"x\",\"password\":\"\"}",
                         "{\"slot\":1,\"priority\":1,\"name\":\"x\",\"ssid\":"
                         "\"x\",\"password\":\"\\u0000hidden\"}",
                         "{\"slot\":1,\"priority\":1,\"name\":\"x\",\"ssid\":"
                         "\"x\",\"password\":\"short\"}",
                         "{\"slot\":1.5,\"priority\":1,\"name\":\"x\",\"ssid\":"
                         "\"x\",\"password\":\"\"}",
                         "{\"slot\":1,\"priority\":1,\"name\":\"x\",\"ssid\":"
                         "\"x\",\"password\":\"\",\"extra\":0}",
                         "{\"slot\":1,\"priority\":1,\"name\":\"x\",\"ssid\":"
                         "\"x\\n\",\"password\":\"\"}"};
    for (unsigned i = 0; i < sizeof(bad) / sizeof(*bad); i++)
        assert(!profile_parse_json(bad[i], &slot, &p));
    char extra[512];
    snprintf(extra, sizeof(extra), "%s trailing", valid);
    assert(!profile_parse_json(extra, &slot, &p));
    puts("PASS: shared browser/serial profile parser malformed, duplicate, "
         "escaped-NUL, numeric and credential validation");
}
