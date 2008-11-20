#	$NetBSD: Makefile,v 1.2 2008/11/20 10:54:38 ad Exp $

.include "../Makefile.inc"

KMOD=	exec_aout

CPPFLAGS+=	-DEXEC_AOUT -DCOREDUMP -DCOMPAT_NOMID

.PATH:	${S}/kern
SRCS=	exec_aout.c

.PATH:	${S}/compat/common
SRCS=	compat_exec.c

.PATH:	${S}/arch/${MACHINE}/${MACHINE}
.PATH:	${S}/arch/${MACHINE_ARCH}/${MACHINE_ARCH}
.PATH:	${S}/arch/${MACHINE_CPU}/${MACHINE_CPU}

.if (exists(${S}/arch/${MACHINE}/${MACHINE}/aout_machdep.c)) || \
    (exists(${S}/arch/${MACHINE_ARCH}/${MACHINE_ARCH}/aout_machdep.c)) || \
    (exists(${S}/arch/${MACHINE_CPU}/${MACHINE_CPU}/aout_machdep.c))
SRCS+=	aout_machdep.c
.endif

.include <bsd.kmodule.mk>
