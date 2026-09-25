/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include <stdatomic.h>
#include "../main/bridge_stats.c"
static atomic_int finished;
static void *writer(void *arg)
{
    bool up=arg!=NULL;
    for(unsigned i=0;i<50000;i++)bridge_count_frame(up,1200);
    atomic_fetch_add(&finished,1);return NULL;
}
int main(void)
{
    bridge_stats_t s;
    s_stats.upload_bytes=UINT64_C(1)<<40;
    bridge_count_frame(true,1514);bridge_get_stats(&s);
    assert(s.upload_bytes==(UINT64_C(1)<<40)+1514);
    s_stats=(bridge_stats_t){0};
    pthread_t up,down;assert(!pthread_create(&up,NULL,writer,(void*)1));assert(!pthread_create(&down,NULL,writer,NULL));
    do {
        bridge_get_stats(&s);
        assert(s.upload_bytes==(uint64_t)s.host_to_wifi*1200);
        assert(s.download_bytes==(uint64_t)s.wifi_to_host*1200);
    } while(atomic_load(&finished)!=2);
    pthread_join(up,NULL);pthread_join(down,NULL);
    for(int i=BRIDGE_DROP_TX;i<=BRIDGE_DROP_RX;i++)bridge_count_drop(i);
    bridge_get_stats(&s);assert(s.host_to_wifi==50000&&s.wifi_to_host==50000);
    assert(s.txdrop==1&&s.reflected==1&&s.poolfail==1&&s.rxdrop==1);
    puts("PASS: production stats module, concurrent snapshots, 64-bit totals and per-cause drops");
}
