/* SPDX-License-Identifier: MIT */
#include "host_observation.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    uint8_t f[128] = {0}; host_observation_t s = {0};
    f[12]=8; f[14]=0x45; f[17]=20; f[26]=192; f[29]=2;
    for (size_t n=0;n<34;n++) {host_observe_frame(&s,f,n);assert(!s.valid4);}
    host_observe_frame(&s,f,34); assert(s.valid4 && s.ipv4[3]==2);
    s=(host_observation_t){0};f[14]=0x44;host_observe_frame(&s,f,34);assert(!s.valid4);
    f[14]=0x46;host_observe_frame(&s,f,34);assert(!s.valid4);
    f[14]=0x45;f[17]=21;host_observe_frame(&s,f,34);assert(!s.valid4);
    memset(f,0,sizeof(f));f[12]=8;f[13]=6;f[15]=1;f[16]=8;f[18]=6;f[19]=4;f[21]=1;f[28]=10;
    host_observe_frame(&s,f,32);assert(!s.valid4);host_observe_frame(&s,f,42);assert(s.valid4);
    s=(host_observation_t){0};f[18]=5;host_observe_frame(&s,f,42);assert(!s.valid4);
    memset(f,0,sizeof(f));f[12]=0x86;f[13]=0xdd;f[14]=0x60;f[22]=0x20;
    host_observe_frame(&s,f,38);assert(!s.valid6);host_observe_frame(&s,f,54);assert(s.valid6);
    s=(host_observation_t){0};f[19]=1;host_observe_frame(&s,f,54);assert(!s.valid6);
    f[19]=0;f[22]=0xfe;host_observe_frame(&s,f,54);assert(!s.valid6);
    uint32_t rng=1;
    for(unsigned k=0;k<10000;k++) {
        for(unsigned i=0;i<sizeof(f);i++){rng=rng*1664525+1013904223;f[i]=rng>>24;}
        host_observe_frame(&s,f,k%sizeof(f));
    }
    puts("PASS: packet observation bounds, malformed headers, preserved IPv6 policy, 10000-input corpus");
}
