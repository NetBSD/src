#	$NetBSD: Makefile,v 1.5 2001/12/12 01:50:06 tv Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/aoutm68k

CPPFLAGS+=	-DSYSVSEM -DSYSVMSG -DSYSVSHM -DCOMPAT_AOUT_M68K -DCOMPAT_43
CPPFLAGS+=	-nostdinc
AFLAGS=		-x assembler-with-cpp -traditional-cpp -D_LOCORE -D_KERNEL \
		-I. -I${S}

NOMAN=	# defined

KMOD=	compat_aoutm68k

SRCS=	lkminit_emul.c
SRCS+=	aoutm68k_exec.c aoutm68k_ioctl.c aoutm68k_stat.c \
	aoutm68k_syscalls.c aoutm68k_sysent.c

.include <bsd.kmod.mk>
