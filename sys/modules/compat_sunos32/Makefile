#	$NetBSD: Makefile,v 1.3 2009/02/14 13:56:41 abs Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

KMOD=	compat_sunos32

CPPFLAGS+=	-DSYSVSHM -DSYSVSEM -DSYSVMSG -DCOMPAT_SUNOS
CPPFLAGS+=	-DCOMPAT_SUNOS32 -DEXEC_AOUT

.PATH:	${S}/compat/sunos32
SRCS+=	sunos32_exec.c sunos32_ioctl.c sunos32_mod.c sunos32_sysent.c
SRCS+=	sunos32_exec_aout.c sunos32_misc.c sunos32_syscalls.c

.if ${MACHINE_ARCH} == "sparc64"
.PATH:	${S}/arch/sparc64/sparc64
SRCS+=	sunos32_machdep.c
.endif

.if ${MACHINE_ARCH} == "sparc"
.PATH:	${S}/arch/sparc/sparc
SRCS+=	sunos32_machdep.c
.endif

.include <bsd.kmodule.mk>
