#include "SYS.h"

	.text
	.align	2
2:
	movl	$-1,%edx
	jmp	PIC_PLT(cerror)
ENTRY(lseek)
	movl	$(SYS_lseek),%eax
	LCALL(7,0)
	jc	2b
	ret
