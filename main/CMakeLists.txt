


idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES 
    Core
    Drivers
    Applications
    Service
    esp_timer 
    esp_wifi 
    nvs_flash 
    fatfs 
    driver
    led_strip
    
     
  

    
)


