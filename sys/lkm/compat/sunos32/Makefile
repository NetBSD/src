#	$NetBSD: Makefile,v 1.1 2001/02/07 13:32:57 mrg Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/sunos32 $S/arch/${MACHINE_ARCH}/${MACHINE_ARCH}

CPPFLAGS+= -DCOMPAT_SUNOS -DSYSVSEM -DSYSVMSG -DSYSVSHM -nostdinc
AFLAGS=		-x assembler-with-cpp -traditional-cpp -D_LOCORE \
		-I. -I${S}

MKMAN=	no
KMOD=	compat_sunos32

# common source files
SRCS+=	lkminit_emul.c
SRCS+=	sunos32_exec.c sunos32_ioctl.c sunos32_misc.c sunos32_syscalls.c sunos32_sysent.c

.include <bsd.kmod.mk>
