idf_component_register(SRCS "bridge.c" "core.c" "settings.c" "control.c" "console.c" "portal.c" "ui.c" "view.c"
 INCLUDE_DIRS "." PRIV_REQUIRES esp_tinyusb esp_wifi esp_netif nvs_flash esp_timer esp_driver_gpio esp_driver_ledc esp_driver_spi esp_lcd st7735 esp_http_server json esp_system)
