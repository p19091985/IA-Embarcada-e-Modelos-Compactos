idf_component_register(
    SRCS
        "main.cpp"
        "soil_app_logic.c"
        "soil_sensor.c"
        "soil_tflite_runner.cc"
        "soil_model_data.cc"
        "bcd_logic.c"
        "bcd_display.c"
        "seven_segment_logic.c"
        "seven_segment_display.c"
        "lcd1602_i2c.c"
        "lcd1602_parallel.c"
    INCLUDE_DIRS "."
    REQUIRES esp_driver_gpio esp_driver_i2c esp_adc esp_rom esp_timer esp-tflite-micro
)
