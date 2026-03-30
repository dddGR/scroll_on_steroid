set(srcs "main.c" "touch_Button.c")
set(include_dir ".")
set(priv_requires "esp_as5600" "esp_hid_bluedroid" "esp_driver_gpio" "esp_driver_uart")

if(IDF_TARGET STREQUAL "esp32s3")
    list(APPEND priv_requires "esp_driver_touch_sens")
endif()

idf_component_register(SRCS ${srcs}
    INCLUDE_DIRS ${include_dir}
    PRIV_REQUIRES ${priv_requires})
