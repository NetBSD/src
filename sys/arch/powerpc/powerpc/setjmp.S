/*	$NetBSD: setjmp.S,v 1.2 2003/07/31 13:59:54 matt Exp $	*/
/*	from:	OpenBSD: setjmp.S,v 1.2 1996/12/28 06:22:18 rahnds Exp 	*/
/* kernel version of this file, does not have signal goop */
/* int setjmp(jmp_buf env) */

#define _NOREGNAMES
#include <machine/asm.h>

#define JMP_r1	0x04
#define JMP_r14	0x08
#define JMP_r15	0x0c
#define JMP_r16	0x10
#define JMP_r17	0x14
#define JMP_r18	0x18
#define JMP_r19	0x1c
#define JMP_r20	0x20
#define JMP_r21	0x24
#define JMP_r22	0x28
#define JMP_r23	0x2c
#define JMP_r24	0x30
#define JMP_r25	0x34
#define JMP_r26	0x38
#define JMP_r27	0x3c
#define JMP_r28	0x40
#define JMP_r29	0x44
#define JMP_r30	0x48
#define JMP_r31	0x4c
#define JMP_lr  0x50
#define JMP_cr  0x54
#define JMP_ctr	0x58
#define JMP_xer	0x5c
#define JMP_sig	0x60


.globl setjmp
setjmp:
	streg %r31, JMP_r31(%r3)
	/* r1, r14-r30 */
	streg %r1,  JMP_r1 (%r3)
	streg %r14, JMP_r14(%r3)
	streg %r15, JMP_r15(%r3)
	streg %r16, JMP_r16(%r3)
	streg %r17, JMP_r17(%r3)
	streg %r18, JMP_r18(%r3)
	streg %r19, JMP_r19(%r3)
	streg %r20, JMP_r20(%r3)
	streg %r21, JMP_r21(%r3)
	streg %r22, JMP_r22(%r3)
	streg %r23, JMP_r23(%r3)
	streg %r24, JMP_r24(%r3)
	streg %r25, JMP_r25(%r3)
	streg %r26, JMP_r26(%r3)
	streg %r27, JMP_r27(%r3)
	streg %r28, JMP_r28(%r3)
	streg %r29, JMP_r29(%r3)
	streg %r30, JMP_r30(%r3)
	/* cr, lr, ctr, xer */
	mfcr %r0
	streg %r0, JMP_cr(%r3)
	mflr %r0
	streg %r0, JMP_lr(%r3)
	mfctr %r0
	streg %r0, JMP_ctr(%r3)
	mfxer %r0
	streg %r0, JMP_xer(%r3)
	/* f14-f31, fpscr */
	li %r3, 0
	blr


.extern sigsetmask
.globl longjmp
longjmp:
	ldreg %r31, JMP_r31(%r3)
	/* r1, r14-r30 */
	ldreg %r1,  JMP_r1 (%r3)
	ldreg %r14, JMP_r14(%r3)
	ldreg %r15, JMP_r15(%r3)
	ldreg %r16, JMP_r16(%r3)
	ldreg %r17, JMP_r17(%r3)
	ldreg %r18, JMP_r18(%r3)
	ldreg %r19, JMP_r19(%r3)
	ldreg %r20, JMP_r20(%r3)
	ldreg %r21, JMP_r21(%r3)
	ldreg %r22, JMP_r22(%r3)
	ldreg %r23, JMP_r23(%r3)
	ldreg %r24, JMP_r24(%r3)
	ldreg %r25, JMP_r25(%r3)
	ldreg %r26, JMP_r26(%r3)
	ldreg %r27, JMP_r27(%r3)
	ldreg %r28, JMP_r28(%r3)
	ldreg %r29, JMP_r29(%r3)
	ldreg %r30, JMP_r30(%r3)
	/* cr, lr, ctr, xer */
	ldreg %r0, JMP_cr(%r3)
	mtcr %r0
	ldreg %r0, JMP_lr(%r3)
	mtlr %r0
	ldreg %r0, JMP_ctr(%r3)
	mtctr %r0
	ldreg %r0, JMP_xer(%r3)
	mtxer %r0
	/* f14-f31, fpscr */
	mr %r3, %r4
	blr
