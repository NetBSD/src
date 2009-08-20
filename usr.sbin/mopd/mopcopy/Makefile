#	$NetBSD: Makefile,v 1.2 2009/08/20 22:26:19 he Exp $

PROG=	mopcopy

.include <bsd.own.mk>
.if ${MACHINE_CPU} == "mips"
CFLAGS+= -DNOAOUT
.endif

.include <bsd.prog.mk>
