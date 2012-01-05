#	$NetBSD: Makefile,v 1.1 2012/01/05 13:26:51 jmcneill Exp $

NETBSDSRCDIR?=	../../../../..

S?=	${NETBSDSRCDIR}/sys
M?=	${S}/modules

.include "${M}/Makefile.inc"
.include "${M}/Makefile.assym"

KMOD=	syscallemu

SRCS=	syscallemu.c
SRCS+=	syscallemu_${MACHINE_ARCH}.c

.include <bsd.kmodule.mk>
