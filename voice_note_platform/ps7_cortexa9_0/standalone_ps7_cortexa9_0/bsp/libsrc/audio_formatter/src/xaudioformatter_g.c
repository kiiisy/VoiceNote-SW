#include "xaudioformatter.h"

XAudioFormatter_Config XAudioFormatter_ConfigTable[] __attribute__ ((section (".drvcfg_sec"))) = {

	{
		"xlnx,audio-formatter-1.0", /* compatible */
		0x43c40000, /* reg */
		0x1, /* xlnx,include-mm2s */
		0x1, /* xlnx,include-s2mm */
		0x2, /* xlnx,max-num-channels-mm2s */
		0x2, /* xlnx,max-num-channels-s2mm */
		0x20, /* xlnx,mm2s-addr-width */
		0x3, /* xlnx,mm2s-dataformat */
		0x0, /* xlnx,packing-mode-mm2s */
		0x0, /* xlnx,packing-mode-s2mm */
		0x20, /* xlnx,s2mm-addr-width */
		0x1, /* xlnx,s2mm-dataformat */
		0x401f, /* interrupts */
		0xf8f01000 /* interrupt-parent */
	},
	 {
		 NULL
	}
};