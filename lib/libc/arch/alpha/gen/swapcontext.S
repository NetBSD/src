#include <machine/asm.h>

NESTED(swapcontext,2,24,ra,0x00030000,0)
	LDGP(pv)
	lda	sp, -24(sp)
	stq	ra, 0(sp)
	stq	a0, 8(sp)
	stq	a1, 16(sp)
	CALL(getcontext)
	ldq	a1, 16(sp)
	ldq	a0, 8(sp)
	ldq	ra, 0(sp)
	bne	v0, 2f	
	stq	ra, 72(a0)		/* Adjust saved pc ... */
	stq	ra, (88 + 26 * 8)(a0)	/* ... and saved ra */
	lda	t0, 24(sp)
	stq	t0, (88 + 30 * 8)(a0)	/* ... and saved sp */
	mov	a1, a0
	CALL(setcontext)
	/* don't check for an error; if we got here, something broke. */
	lda	sp, 24(sp)
2:	RET
	END(swapcontext)