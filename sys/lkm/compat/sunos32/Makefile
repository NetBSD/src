#	$NetBSD: Makefile,v 1.3 2001/12/12 01:50:08 tv Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/sunos32 $S/arch/${MACHINE_ARCH}/${MACHINE_ARCH}

CPPFLAGS+= -DCOMPAT_SUNOS -DSYSVSEM -DSYSVMSG -DSYSVSHM -nostdinc
AFLAGS=		-x assembler-with-cpp -traditional-cpp -D_LOCORE \
		-I. -I${S}

NOMAN=	# defined
KMOD=	compat_sunos32

# common source files
SRCS+=	lkminit_emul.c
SRCS+=	sunos32_exec.c sunos32_ioctl.c sunos32_misc.c sunos32_syscalls.c sunos32_sysent.c

.if (${MACHINE} == "sparc64")
SRCS+=	sunos32_machdep.c sunos_sigcode.s
.endif

.include <bsd.kmod.mk>
