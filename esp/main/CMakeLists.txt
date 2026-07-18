idf_component_register(SRCS "mike_main.c"
                       PRIV_REQUIRES esp_driver_uart esp_driver_rmt esp_timer
                       INCLUDE_DIRS "")
                       