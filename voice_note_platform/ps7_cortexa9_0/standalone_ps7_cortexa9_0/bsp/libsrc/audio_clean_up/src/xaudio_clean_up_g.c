#include "xaudio_clean_up.h"

XAudio_clean_up_Config XAudio_clean_up_ConfigTable[] __attribute__ ((section (".drvcfg_sec"))) = {

	{
		"xlnx,audio-clean-up-1.0", /* compatible */
		0x43ca0000 /* reg */
	},
	 {
		 NULL
	}
};