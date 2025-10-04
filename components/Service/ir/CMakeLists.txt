idf_component_register(
    SRCS 
        "ir_encoder.c"
        "ir_common.c"
        "ir_tx.c"
        "ir_rx.c"
        "ir_storage.c"
        "protocol_nec.c"
        "protocol_rc6.c"
        "protocol_rc5.c"
        "protocol_samsung32.c"
        "protocol_sony.c"
        "ir_burst.c"
    INCLUDE_DIRS "include"
)