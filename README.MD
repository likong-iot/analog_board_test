# 程序烧录
## 固件位置
 bootloader:build/bootloader/bootloader.bin
 paration:build/paration_table/paration_table.bin
 app:build/analog_board_test.bin
## 程序烧录地址

    bootloader.bin：0x1000
    partition-table.bin：0x8000
    应用 app：0x10000