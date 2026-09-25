// SPDX-License-Identifier: MIT
int main(void){
 settings_t s,read;settings_load(&s);assert(settings_valid(&s));
 profile_t p={.name="Original",.ssid="SSID",.pass="password",.priority=50};s.p[0]=p;assert(settings_save(&s)==ESP_OK);
 settings_load(&read);assert(!strcmp(read.p[0].name,"Original"));
 strcpy(s.p[0].name,"Replacement");fail_commit=true;assert(settings_save(&s)!=ESP_OK);fail_commit=false;
 settings_load(&read);assert(!strcmp(read.p[0].name,"Original"));
 assert(settings_stage(0,&s.p[0])==ESP_OK);int slot;profile_t candidate;assert(settings_take_candidate(&slot,&candidate));assert(slot==0&&!strcmp(candidate.name,"Replacement"));assert(!settings_take_candidate(&slot,&candidate));
 settings_load(&read);assert(!strcmp(read.p[0].name,"Original")); // warm reset mid-trial preserves working profile
 ((settings_t*)committed[0])->version=999;settings_load(&read);assert(!settings_store_ok());assert(settings_save(&s)==ESP_ERR_INVALID_STATE);
 assert(settings_reset()==ESP_OK);settings_load(&read);assert(settings_store_ok()&&!read.p[0].ssid[0]);
 puts("PASS: actual NVS module defaults, failed commit, candidate consume, interrupted trial, schema rejection, explicit reset (mock NVS)");
}
