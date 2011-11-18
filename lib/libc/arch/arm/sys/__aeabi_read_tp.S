#include "SYS.h"

ENTRY(__aeabi_read_tp)
	mrc p15, 0, r0, c13, c0, 3
	RET
