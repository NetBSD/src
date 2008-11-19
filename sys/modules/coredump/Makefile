#	$NetBSD: Makefile,v 1.1 2008/11/19 18:36:08 ad Exp $

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
