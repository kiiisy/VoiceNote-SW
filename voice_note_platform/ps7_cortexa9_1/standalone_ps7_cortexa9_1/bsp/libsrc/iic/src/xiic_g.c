#include "xiic.h"

XIic_Config XIic_ConfigTable[] __attribute__ ((section (".drvcfg_sec"))) = {

	{
		"xlnx,axi-iic-2.1", /* compatible */
		0x43c30000, /* reg */
		0x0, /* xlnx,ten-bit-adr */
		0x1, /* xlnx,gpo-width */
		0x4021, /* interrupts */
		0xf8f01000 /* interrupt-parent */
	},
	{
		"xlnx,axi-iic-2.1", /* compatible */
		0x43c50000, /* reg */
		0x0, /* xlnx,ten-bit-adr */
		0x1, /* xlnx,gpo-width */
		0x4023, /* interrupts */
		0xf8f01000 /* interrupt-parent */
	},
	 {
		 NULL
	}
};