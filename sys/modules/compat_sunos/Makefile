#	$NetBSD: Makefile,v 1.1 2008/11/19 18:36:08 ad Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

KMOD=	compat_sunos

CPPFLAGS+=	-DSYSVSHM -DSYSVSEM -DSYSVIPC -DCOMPAT_SUNOS
CPPFLAGS+=	-DEXEC_AOUT

.PATH:	${S}/compat/sunos
SRCS+=	sunos_exec.c sunos_ioctl.c sunos_mod.c sunos_sysent.c
SRCS+=	sunos_exec_aout.c sunos_misc.c sunos_syscalls.c

.if ${MACHINE} == "sparc64"
.PATH:	${S}/arch/sparc64/sparc64
SRCS+=	sunos_machdep.c
.endif

.if ${MACHINE} == "sparc"
.PATH:	${S}/arch/sparc/sparc
SRCS+=	sunos_machdep.c
.endif

.include <bsd.kmodule.mk>
