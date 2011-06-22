#ifndef _CFI_H_
#define _CFI_H_

enum {
	CFI_QUERY_MODE_ADDRESS = 0x55, /* some devices accept anything */
	CFI_QUERY_DATA = 0x98,

	CFI_READ_ARRAY_MODE_ADDRESS = 0x00, /* could be anything */
	CFI_READ_ARRAY_MODE_DATA = 0xf0,    /* also can be 0xff */

	CFI_ADDR_IDENTIFICATION_STRING = 0x10,
	CFI_ADDR_SYS_INTERFACE_INFO = 0x1b,
	CFI_ADDR_DEV_GEOMETRY = 0x27
};

#endif
