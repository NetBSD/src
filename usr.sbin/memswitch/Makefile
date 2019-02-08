#	$NetBSD: Makefile,v 1.3 2019/02/08 08:55:35 isaki Exp $

.if ${MACHINE} == "x68k"
PROG=	memswitch
SRCS=	memswitch.c properties.c methods.c
.endif

MAN=	memswitch.8
MANSUBDIR=	/x68k

.include <bsd.prog.mk>
