
cmake_minimum_required(VERSION 3.15)
project(bsp)
        
set(DRIVER_TARGETS audio_clean_up;audio_formatter;gpio;iic;i2srx;i2stx;coresightps_dcc;devcfg;dmaps;gpiops;scugic;qspips;scutimer;scuwdt;sdps;spips;uartps;usbps;xadcps)
set(DRIVER_LOCATIONS /home/user/work/FY25/03_project/VoiceNote/sw/voice_note_platform/ps7_cortexa9_0/standalone_ps7_cortexa9_0/bsp/hw_artifacts/drivers/audio_clean_up_v1_0;/home/user/Xilinx/Vitis/2024.2/data/embeddedsw/XilinxProcessorIPLib/drivers/audio_formatter_v1_3;/home/user/Xilinx/Vitis/2024.2/data/embeddedsw/XilinxProcessorIPLib/drivers/gpio_v4_12;/home/user/Xilinx/Vitis/2024.2/data/embeddedsw/XilinxProcessorIPLib/drivers/iic_v3_12;/home/user/Xilinx/Vitis/2024.2/data/embeddedsw/XilinxProcessorIPLib/drivers/i2srx_v2_3;/home/user/Xilinx/Vitis/2024.2/data/embeddedsw/XilinxProcessorIPLib/drivers/i2stx_v2_3;/home/user/Xilinx/Vitis/2024.2/data/embeddedsw/XilinxProcessorIPLib/drivers/coresightps_dcc_v1_11;/home/user/Xilinx/Vitis/2024.2/data/embeddedsw/XilinxProcessorIPLib/drivers/devcfg_v3_8;/home/user/Xilinx/Vitis/2024.2/data/embeddedsw/XilinxProcessorIPLib/drivers/dmaps_v2_11;/home/user/Xilinx/Vitis/2024.2/data/embeddedsw/XilinxProcessorIPLib/drivers/gpiops_v3_13;/home/user/Xilinx/Vitis/2024.2/data/embeddedsw/XilinxProcessorIPLib/drivers/scugic_v5_4;/home/user/Xilinx/Vitis/2024.2/data/embeddedsw/XilinxProcessorIPLib/drivers/qspips_v3_13;/home/user/Xilinx/Vitis/2024.2/data/embeddedsw/XilinxProcessorIPLib/drivers/scutimer_v2_6;/home/user/Xilinx/Vitis/2024.2/data/embeddedsw/XilinxProcessorIPLib/drivers/scuwdt_v2_6;/home/user/Xilinx/Vitis/2024.2/data/embeddedsw/XilinxProcessorIPLib/drivers/sdps_v4_4;/home/user/Xilinx/Vitis/2024.2/data/embeddedsw/XilinxProcessorIPLib/drivers/spips_v3_11;/home/user/Xilinx/Vitis/2024.2/data/embeddedsw/XilinxProcessorIPLib/drivers/uartps_v3_15;/home/user/Xilinx/Vitis/2024.2/data/embeddedsw/XilinxProcessorIPLib/drivers/usbps_v2_9;/home/user/Xilinx/Vitis/2024.2/data/embeddedsw/XilinxProcessorIPLib/drivers/xadcps_v2_7)

list(LENGTH DRIVER_TARGETS no_of_drivers)
set(index 0)

while(${index} LESS ${no_of_drivers})
    list(GET DRIVER_TARGETS ${index} drv)
    list(GET DRIVER_LOCATIONS ${index} drv_dir)
    set(exp_dir "${drv_dir}/examples")
    add_custom_target(
        ${drv}_example ALL
        COMMAND lopper -f -O /home/user/work/FY25/03_project/VoiceNote/sw/voice_note_platform/ps7_cortexa9_0/standalone_ps7_cortexa9_0/bsp/libsrc/${drv} /home/user/work/FY25/03_project/VoiceNote/sw/voice_note_platform/ps7_cortexa9_0/standalone_ps7_cortexa9_0/bsp/hw_artifacts/ps7_cortexa9_0_baremetal.dts -- bmcmake_metadata_xlnx ps7_cortexa9_0 ${exp_dir} drvcmake_metadata
        BYPRODUCTS x${drv}_exlist.yaml)
    MATH(EXPR index "${index}+1")
endwhile()
