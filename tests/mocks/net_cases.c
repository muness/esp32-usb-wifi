// SPDX-License-Identifier: MIT
static void released(void *cookie,void *ctx){(void)ctx;assert(cookie==(void*)123);free_count++;}
int main(void){
 tinyusb_net_config_t cfg={.free_tx_buffer=released};assert(tinyusb_net_init(&cfg)==ESP_OK);
 uint8_t bytes[100]={0};
 schedule=0;assert(tinyusb_net_send_sync(bytes,100,(void*)123,20)==ESP_OK);assert(free_count==1);
 schedule=1;assert(tinyusb_net_send_sync(bytes,100,(void*)123,20)==ESP_ERR_TIMEOUT);assert(free_count==1);run_deferred();assert(free_count==1);
 /* Callback begins exactly as wait times out: success must not return timeout,
    otherwise bridge caller releases the already-freed frame a second time. */
 schedule=2;assert(tinyusb_net_send_sync(bytes,100,(void*)123,20)==ESP_OK);assert(free_count==2);
 schedule=0;allow_tx=0;assert(tinyusb_net_send_sync(bytes,100,(void*)123,20)==ESP_FAIL);assert(free_count==2);
 allow_tx=1;assert(tinyusb_net_send_sync(bytes,100,(void*)123,20)==ESP_OK);assert(free_count==3);
 tinyusb_net_deinit();puts("PASS: actual TinyUSB wrapper success, cancellation, deadline/copy race, backpressure, recovery ownership");
}
