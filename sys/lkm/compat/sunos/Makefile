#	$NetBSD: Makefile,v 1.6 2000/12/13 11:18:39 enami Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/sunos $S/arch/${MACHINE_ARCH}/${MACHINE_ARCH}

CPPFLAGS+= -DCOMPAT_SUNOS -DSYSVSEM -DSYSVMSG -DSYSVSHM -nostdinc
AFLAGS=		-x assembler-with-cpp -traditional-cpp -D_LOCORE \
		-I. -I${S}

MKMAN=	no
KMOD=	compat_sunos

# common source files
SRCS+=	lkminit_emul.c
SRCS+=	sunos_exec.c sunos_ioctl.c sunos_misc.c sunos_syscalls.c sunos_sysent.c

# MD files
.if ${MACHINE_ARCH} == "m68k"
SRCS+=	sunos_sigcode.s
.endif

.include <bsd.kmod.mk>
