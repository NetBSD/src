#	$NetBSD: Makefile,v 1.2.4.2 2002/01/08 00:32:56 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/aout

CPPFLAGS+=	-DSYSVSEM -DSYSVMSG -DSYSVSHM -DCOMPAT_AOUT
CPPFLAGS+=	-nostdinc
AFLAGS=		-x assembler-with-cpp -traditional-cpp -D_LOCORE -D_KERNEL \
		-I. -I${S}

KMOD=	compat_aout

SRCS=	lkminit_emul.c
SRCS+=	aout_exec.c aout_misc.c aout_syscalls.c aout_sysent.c

.include <bsd.kmod.mk>
