idf_component_register(
    SRCS "ui_sd.c"
    INCLUDE_DIRS "include"
    REQUIRES 
   Drivers
    Applications
    fatfs
    hb_sd
)
