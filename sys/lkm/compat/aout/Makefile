#	$NetBSD: Makefile,v 1.1 2000/12/08 20:15:13 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/aout

CPPFLAGS+=	-DVM86 -DSYSVSEM -DSYSVMSG -DSYSVSHM -DCOMPAT_AOUT
CPPFLAGS+=	-nostdinc
AFLAGS=         -x assembler-with-cpp -traditional-cpp -D_LOCORE -D_KERNEL \
                -I. -I${S}

MKMAN=	no

KMOD=	compat_aout

SRCS=	lkminit_emul.c
SRCS+=	aout_exec.c aout_misc.c aout_syscalls.c aout_sysent.c

.include <bsd.kmod.mk>
