# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "/home/user/work/FY25/03_project/VoiceNote/sw/voice_note_platform/zynq_fsbl/zynq_fsbl_bsp/include/diskio.h"
  "/home/user/work/FY25/03_project/VoiceNote/sw/voice_note_platform/zynq_fsbl/zynq_fsbl_bsp/include/ff.h"
  "/home/user/work/FY25/03_project/VoiceNote/sw/voice_note_platform/zynq_fsbl/zynq_fsbl_bsp/include/ffconf.h"
  "/home/user/work/FY25/03_project/VoiceNote/sw/voice_note_platform/zynq_fsbl/zynq_fsbl_bsp/include/sleep.h"
  "/home/user/work/FY25/03_project/VoiceNote/sw/voice_note_platform/zynq_fsbl/zynq_fsbl_bsp/include/xilffs.h"
  "/home/user/work/FY25/03_project/VoiceNote/sw/voice_note_platform/zynq_fsbl/zynq_fsbl_bsp/include/xilffs_config.h"
  "/home/user/work/FY25/03_project/VoiceNote/sw/voice_note_platform/zynq_fsbl/zynq_fsbl_bsp/include/xilrsa.h"
  "/home/user/work/FY25/03_project/VoiceNote/sw/voice_note_platform/zynq_fsbl/zynq_fsbl_bsp/include/xiltimer.h"
  "/home/user/work/FY25/03_project/VoiceNote/sw/voice_note_platform/zynq_fsbl/zynq_fsbl_bsp/include/xtimer_config.h"
  "/home/user/work/FY25/03_project/VoiceNote/sw/voice_note_platform/zynq_fsbl/zynq_fsbl_bsp/lib/libxilffs.a"
  "/home/user/work/FY25/03_project/VoiceNote/sw/voice_note_platform/zynq_fsbl/zynq_fsbl_bsp/lib/libxilrsa.a"
  "/home/user/work/FY25/03_project/VoiceNote/sw/voice_note_platform/zynq_fsbl/zynq_fsbl_bsp/lib/libxiltimer.a"
  )
endif()
