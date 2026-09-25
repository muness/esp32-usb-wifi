// SPDX-License-Identifier: MIT
#include "core.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
typedef int esp_err_t;
typedef int nvs_handle_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_INVALID_STATE -2
#define ESP_ERR_INVALID_ARG -3
#define ESP_ERR_NVS_NOT_FOUND -4
#define NVS_READONLY 0
#define NVS_READWRITE 1
static unsigned char committed[2][2048], staged[2][2048];
static size_t sizes[2], staged_sizes[2];
static bool fail_commit;
static int key(const char *s){return !strcmp(s,"config")?0:1;}
static esp_err_t nvs_open(const char *ns,int mode,nvs_handle_t *h){(void)ns;(void)mode;*h=1;memcpy(staged,committed,sizeof(staged));memcpy(staged_sizes,sizes,sizeof(sizes));return ESP_OK;}
static void nvs_close(nvs_handle_t h){(void)h;}
static esp_err_t nvs_get_blob(nvs_handle_t h,const char *s,void *p,size_t *n){(void)h;int i=key(s);if(!sizes[i])return ESP_ERR_NVS_NOT_FOUND;if(*n<sizes[i])return ESP_FAIL;memcpy(p,committed[i],sizes[i]);*n=sizes[i];return ESP_OK;}
static esp_err_t nvs_set_blob(nvs_handle_t h,const char *s,const void *p,size_t n){(void)h;assert(n<=2048);int i=key(s);memcpy(staged[i],p,n);staged_sizes[i]=n;return ESP_OK;}
static esp_err_t nvs_erase_key(nvs_handle_t h,const char *s){(void)h;staged_sizes[key(s)]=0;return ESP_OK;}
static esp_err_t nvs_erase_all(nvs_handle_t h){(void)h;memset(staged_sizes,0,sizeof(staged_sizes));return ESP_OK;}
static esp_err_t nvs_commit(nvs_handle_t h){(void)h;if(fail_commit)return ESP_FAIL;memcpy(committed,staged,sizeof(staged));memcpy(sizes,staged_sizes,sizeof(sizes));return ESP_OK;}
