# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "/home/user/work/FY25/03_project/VoiceNote/sw/voice_note_platform/ps7_cortexa9_0/standalone_ps7_cortexa9_0/bsp/include/diskio.h"
  "/home/user/work/FY25/03_project/VoiceNote/sw/voice_note_platform/ps7_cortexa9_0/standalone_ps7_cortexa9_0/bsp/include/ff.h"
  "/home/user/work/FY25/03_project/VoiceNote/sw/voice_note_platform/ps7_cortexa9_0/standalone_ps7_cortexa9_0/bsp/include/ffconf.h"
  "/home/user/work/FY25/03_project/VoiceNote/sw/voice_note_platform/ps7_cortexa9_0/standalone_ps7_cortexa9_0/bsp/include/sleep.h"
  "/home/user/work/FY25/03_project/VoiceNote/sw/voice_note_platform/ps7_cortexa9_0/standalone_ps7_cortexa9_0/bsp/include/xilffs.h"
  "/home/user/work/FY25/03_project/VoiceNote/sw/voice_note_platform/ps7_cortexa9_0/standalone_ps7_cortexa9_0/bsp/include/xilffs_config.h"
  "/home/user/work/FY25/03_project/VoiceNote/sw/voice_note_platform/ps7_cortexa9_0/standalone_ps7_cortexa9_0/bsp/include/xiltimer.h"
  "/home/user/work/FY25/03_project/VoiceNote/sw/voice_note_platform/ps7_cortexa9_0/standalone_ps7_cortexa9_0/bsp/include/xtimer_config.h"
  "/home/user/work/FY25/03_project/VoiceNote/sw/voice_note_platform/ps7_cortexa9_0/standalone_ps7_cortexa9_0/bsp/lib/libxilffs.a"
  "/home/user/work/FY25/03_project/VoiceNote/sw/voice_note_platform/ps7_cortexa9_0/standalone_ps7_cortexa9_0/bsp/lib/libxiltimer.a"
  )
endif()
