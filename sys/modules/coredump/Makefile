#	$NetBSD: Makefile,v 1.1.4.2 2009/01/17 13:29:29 mjf Exp $

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
SRCS+=	core_machdep.c

.include <bsd.kmodule.mk>
