/*	$NetBSD: __sigsetjmp14.S,v 1.1 2014/09/19 17:36:25 matt Exp $	*/

#include "SYS.h"
#include "assym.h"

#if defined(LIBC_SCCS)
__RCSID("$NetBSD: __sigsetjmp14.S,v 1.1 2014/09/19 17:36:25 matt Exp $")
#endif

ENTRY(__sigsetjmp14)
	bnez	a1, 1f
	jump	PLT(_C_LABEL(_setjmp))
1:	jump	PLT(_C_LABEL(__setjmp14))
END(__sigsetjmp14)

ENTRY(__siglongjmp14)
	REG_L	a2, JB_MAGIC(a0)
	bnez	a2, 1f
	jump	PLT(_C_LABEL(_longjmp))
1:	jump	PLT(_C_LABEL(__longjmp14))
END(__siglongjmp14)
