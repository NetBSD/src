#	$NetBSD: Makefile,v 1.9 2002/02/10 15:41:58 mrg Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/sunos $S/arch/${MACHINE}/${MACHINE} $S/arch/${MACHINE_ARCH}/${MACHINE_ARCH} $S/compat/common

CPPFLAGS+= -DCOMPAT_SUNOS -DSYSVSEM -DSYSVMSG -DSYSVSHM -nostdinc
AFLAGS=		-x assembler-with-cpp -traditional-cpp -D_LOCORE \
		-I. -I${S}

KMOD=	compat_sunos

# common source files
SRCS+=	lkminit_emul.c
SRCS+=	sunos_exec.c sunos_ioctl.c sunos_misc.c sunos_syscalls.c sunos_sysent.c

# MD files
.if ${MACHINE_ARCH} == "m68k"
SRCS+=	sunos_sigcode.s
.endif

.if (${MACHINE} == "sparc64" || ${MACHINE} == "sparc")
SRCS+=	sunos_machdep.c sunos_sigcode.s
.endif

.include <bsd.kmod.mk>
