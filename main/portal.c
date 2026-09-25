// SPDX-License-Identifier: MIT
#include "app.h"
#include "cJSON.h"
#include "esp_http_server.h"
#include "esp_netif.h"
#include "esp_random.h"
#include "esp_wifi.h"
#include <stdio.h>
#include <string.h>
static char ap_ssid[25], ap_pass[17], token[33];
void portal_identity(char *ssid, char *pass) {
  uint8_t mac[6];
  bridge_get_mac(mac);
  snprintf(ap_ssid, sizeof(ap_ssid), "TDongle-%02X%02X%02X", mac[3], mac[4],
           mac[5]);
  for (int i = 0; i < 16; i++)
    ap_pass[i] = "abcdefghijkmnpqrstuvwxyz23456789"[esp_random() % 32];
  ap_pass[16] = 0;
  for (int i = 0; i < 32; i++)
    token[i] = "0123456789abcdef"[esp_random() % 16];
  token[32] = 0;
  strcpy(ssid, ap_ssid);
  strcpy(pass, ap_pass);
}
static esp_err_t headers(httpd_req_t *r) {
  httpd_resp_set_hdr(r, "Cache-Control", "no-store");
  httpd_resp_set_hdr(r, "X-Content-Type-Options", "nosniff");
  httpd_resp_set_hdr(r, "X-Frame-Options", "DENY");
  httpd_resp_set_hdr(
      r, "Content-Security-Policy",
      "default-src 'none'; script-src 'unsafe-inline'; style-src "
      "'unsafe-inline'; connect-src 'self'; frame-ancestors 'none'");
  return ESP_OK;
}
static esp_err_t page(httpd_req_t *r) {
  headers(r);
  httpd_resp_set_type(r, "text/html");
  const char *start =
      "<!doctype html><meta name=viewport "
      "content='width=device-width,initial-scale=1'><title>T-Dongle "
      "setup</title><style>body{font:18px sans-serif;max-width:28em;margin:1em "
      "auto;padding:1em;background:#101820;color:white}input,button{display:"
      "block;font:inherit;margin:.5em "
      "0;padding:.5em;max-width:95%}button{background:#bfefff}small{display:"
      "block}</style><h1>T-Dongle setup</h1><p>Forwarding is paused. This "
      "setup network expires after 10 minutes.</p><form id=f><label>Profile "
      "slot (1–8)<input name=slot type=number min=1 max=8 value=1 "
      "required></label><label>Profile name<input name=name maxlength=24 "
      "required></label><label>2.4 GHz SSID<input name=ssid maxlength=32 "
      "required></label><label>Password (empty for open)<input name=password "
      "type=password maxlength=63 "
      "autocomplete=new-password></label><label>Priority (0–100)<input "
      "name=priority type=number min=0 max=100 value=50 "
      "required></label><button>Save and test "
      "connection</button></form><button id=cancel>Cancel setup</button><p "
      "id=result></p><small>Save reboots into adapter mode. Replacement is "
      "committed only after 10 seconds of association within a 45 second "
      "trial. This does not verify Internet access. If it fails, previous "
      "saved credentials remain.</small><script>const token='";
  httpd_resp_send_chunk(r, start, HTTPD_RESP_USE_STRLEN);
  httpd_resp_send_chunk(r, token, HTTPD_RESP_USE_STRLEN);
  const char *end =
      "';async function send(path,data){try{const r=await "
      "fetch(path,{method:'POST',headers:{'Content-Type':'application/"
      "json','X-Setup-Token':token},body:JSON.stringify(data)});document."
      "getElementById('result').textContent=await "
      "r.text();}catch(e){document.getElementById('result').textContent='"
      "Connection closed. Check the dongle "
      "screen.';}}document.getElementById('f').onsubmit=e=>{e.preventDefault();"
      "let d=Object.fromEntries(new "
      "FormData(e.target));d.slot=Number(d.slot);d.priority=Number(d.priority);"
      "send('/"
      "save',d);e.target.password.value='';};document.getElementById('cancel')."
      "onclick=()=>send('/cancel',{});</script>";
  httpd_resp_send_chunk(r, end, HTTPD_RESP_USE_STRLEN);
  return httpd_resp_send_chunk(r, NULL, 0);
}
static esp_err_t post(httpd_req_t *r) {
  headers(r);
  char supplied[40];
  if (httpd_req_get_hdr_value_str(r, "X-Setup-Token", supplied,
                                  sizeof(supplied)) != ESP_OK ||
      strcmp(supplied, token))
    return httpd_resp_send_err(r, HTTPD_403_FORBIDDEN, "Invalid setup token");
  if (r->content_len < 2 || r->content_len > 400)
    return httpd_resp_send_err(r, HTTPD_400_BAD_REQUEST,
                               "Body must be 2..400 bytes");
  char body[401];
  size_t n = 0;
  while (n < r->content_len) {
    int got = httpd_req_recv(r, body + n, r->content_len - n);
    if (got <= 0)
      return httpd_resp_send_err(r, HTTPD_408_REQ_TIMEOUT,
                                 "Incomplete request");
    n += got;
  }
  body[n] = 0;
  if (memchr(body, 0, n))
    return httpd_resp_send_err(r, HTTPD_400_BAD_REQUEST, "Invalid body");
  bool ok;
  if (!strcmp(r->uri, "/cancel"))
    ok = control_submit("cancel");
  else {
    /* Reject controls and escaped NUL before cJSON can truncate strings. */
    for (size_t i = 0; i < n; i++)
      if ((unsigned char)body[i] < 32)
        return httpd_resp_send_err(r, HTTPD_400_BAD_REQUEST,
                                   "Compact JSON required");
    if (strstr(body, "\\u"))
      return httpd_resp_send_err(r, HTTPD_400_BAD_REQUEST,
                                 "Use printable ASCII fields");
    const char *end;
    cJSON *o = cJSON_ParseWithOpts(body, &end, true);
    if (!o)
      return httpd_resp_send_err(r, HTTPD_400_BAD_REQUEST, "Invalid JSON");
    cJSON_Delete(o);
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "profile %s", body);
    ok = control_submit(cmd);
    memset(cmd, 0, sizeof(cmd));
  }
  memset(body, 0, sizeof(body));
  if (!ok)
    return httpd_resp_send_err(r, HTTPD_500_INTERNAL_SERVER_ERROR,
                               "Busy; retry");
  return httpd_resp_sendstr(r,
                            "Request queued. Check screen for validation; "
                            "valid save/cancel disconnects setup and reboots.");
}
void portal_start(void) {
  /* Only setup mode creates an esp_netif. Adapter mode has no LwIP interface.
   */
  esp_wifi_disconnect();
  esp_wifi_stop();
  bridge_clear_addresses();
  ESP_ERROR_CHECK(esp_netif_init());
  assert(esp_netif_create_default_wifi_ap());
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  wifi_config_t c = {0};
  strcpy((char *)c.ap.ssid, ap_ssid);
  strcpy((char *)c.ap.password, ap_pass);
  c.ap.ssid_len = strlen(ap_ssid);
  c.ap.channel = 1;
  c.ap.authmode = WIFI_AUTH_WPA2_PSK;
  c.ap.max_connection = 2;
  c.ap.pmf_cfg.capable = true;
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &c));
  ESP_ERROR_CHECK(esp_wifi_start());
  httpd_config_t h = HTTPD_DEFAULT_CONFIG();
  h.stack_size = 6144;
  h.max_open_sockets = 3;
  h.lru_purge_enable = true;
  h.recv_wait_timeout = 3;
  h.send_wait_timeout = 3;
  httpd_handle_t server;
  if (httpd_start(&server, &h) != ESP_OK) {
    mgmt_write("ERR setup HTTP start failed\r\n");
    return;
  }
  httpd_uri_t root = {.uri = "/", .method = HTTP_GET, .handler = page},
              save = {.uri = "/save", .method = HTTP_POST, .handler = post},
              cancel = {.uri = "/cancel", .method = HTTP_POST, .handler = post};
  httpd_register_uri_handler(server, &root);
  httpd_register_uri_handler(server, &save);
  httpd_register_uri_handler(server, &cancel);
}
