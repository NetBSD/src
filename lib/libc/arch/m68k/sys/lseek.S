#include "SYS.h"

	.even
err:
	movl	#-1,d1
	movl	d0,sp@-
	jra	cerror
ENTRY(lseek)
	movl	#SYS_lseek,d0
	trap	#0
	jcs	err
	rts
