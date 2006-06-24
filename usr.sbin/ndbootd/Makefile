#	$NetBSD: Makefile,v 1.2 2006/06/24 05:21:11 mrg Exp $

PROG=	ndbootd
SRCS=	ndbootd.c
MAN=	ndbootd.8

CPPFLAGS+=-DHAVE_CONFIG_H -I${.CURDIR}

.include <bsd.own.mk>

.if ${HAVE_GCC} == 4 && ${MACHINE_ARCH} == "m68000"
COPTS+=	-O0
.endif

.include <bsd.prog.mk>
