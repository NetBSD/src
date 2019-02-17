#	$NetBSD: Makefile,v 1.4 2019/02/17 04:05:46 rin Exp $

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
.if ${MACHINE_ARCH} == "powerpc64"
.PATH: ${S}/arch/powerpc/powerpc
.endif
SRCS+=	core_machdep.c

WARNS=	3

.include <bsd.kmodule.mk>
