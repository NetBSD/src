#	$NetBSD: Makefile,v 1.5 2008/04/27 03:06:46 tsutsui Exp $

USE_FORT?= yes	# network server

PROG=	ndbootd
SRCS=	ndbootd.c
MAN=	ndbootd.8

CPPFLAGS+=-DHAVE_CONFIG_H -I${.CURDIR}

.if ${MACHINE_ARCH} == "m68000"
COPTS.ndbootd.c+=	-fno-tree-ter -fno-tree-lrs
.endif

.include <bsd.own.mk>
.include <bsd.prog.mk>
