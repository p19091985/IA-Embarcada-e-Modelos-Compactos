idf_component_register(
    SRCS
        "main.c"
        "dht22.c"
        "lcd1602_i2c.c"
    INCLUDE_DIRS "."
    REQUIRES esp_driver_gpio esp_driver_i2c esp_rom esp_timer
)
