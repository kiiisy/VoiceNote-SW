#include "xi2srx.h"

XI2srx_Config XI2srx_ConfigTable[] __attribute__ ((section (".drvcfg_sec"))) = {

	{
		"xlnx,i2s-receiver-1.0", /* compatible */
		0x43c00000, /* reg */
		0x10, /* xlnx,dwidth */
		0x1, /* xlnx,is-master */
		0x1, /* xlnx,num-channels */
		0x0 /* xlnx,32bit-lr */
	},
	 {
		 NULL
	}
};