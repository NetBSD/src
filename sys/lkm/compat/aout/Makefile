#	$NetBSD: Makefile,v 1.5 2001/12/12 11:44:44 lukem Exp $

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
