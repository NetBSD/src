/*	$NetBSD: profile.h,v 1.4 1999/03/05 07:59:14 tsubai Exp $	*/

#define	_MCOUNT_DECL	void mcount

#ifdef PIC
#define _PLT "@plt"
#else
#define _PLT
#endif

#define MCOUNT __asm("		\
	.globl _mcount;		\
_mcount:			\
	stwu	1,-64(1);	\
	stw	3,16(1);	\
	stw	4,20(1);	\
	stw	5,24(1);	\
	stw	6,28(1);	\
	stw	7,32(1);	\
	stw	8,36(1);	\
	stw	9,40(1);	\
	stw	10,44(1);	\
				\
	mflr	4;		\
	stw	4,48(1);	\
	lwz	3,68(1);	\
	bl	mcount" _PLT ";	\
	lwz	3,68(1);	\
	mtlr	3;		\
	lwz	4,48(1);	\
	mtctr	4;		\
				\
	lwz	3,16(1);	\
	lwz	4,20(1);	\
	lwz	5,24(1);	\
	lwz	6,28(1);	\
	lwz	7,32(1);	\
	lwz	8,36(1);	\
	lwz	9,40(1);	\
	lwz	10,44(1);	\
	addi	1,1,64;		\
	bctr;			");

#ifdef _KERNEL
#define	MCOUNT_ENTER
#define	MCONT_EXIT
#endif
