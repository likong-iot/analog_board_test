# TCA9535 I/O扩展器驱动组件

idf_component_register(
    SRCS "tca9535.c"
    INCLUDE_DIRS "include"
    REQUIRES driver log
    PRIV_REQUIRES main
)
