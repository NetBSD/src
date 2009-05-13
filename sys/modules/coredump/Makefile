#	$NetBSD: Makefile,v 1.1.8.1 2009/05/13 17:22:18 jym Exp $

.include "../Makefile.inc"

KMOD=	coredump

CPPFLAGS+=	-DCOREDUMP

.PATH:	${S}/kern
SRCS=	kern_core.c core_netbsd.c

.PATH:	${S}/uvm
SRCS+=	uvm_coredump.c

.PATH:	${S}/arch/${MACHINE}/${MACHINE}
.PATH:	${S}/arch/${MACHINE_ARCH}/${MACHINE_ARCH}
.PATH:	${S}/arch/${MACHINE_CPU}/${MACHINE_CPU}
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
.PATH:	${S}/arch/x86/x86
.endif
SRCS+=	core_machdep.c

.include <bsd.kmodule.mk>
