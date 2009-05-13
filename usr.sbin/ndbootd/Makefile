#	$NetBSD: Makefile,v 1.5.8.1 2009/05/13 19:20:31 jym Exp $

WARNS?=	1	# XXX -Wsign-compare -Wshadow

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
