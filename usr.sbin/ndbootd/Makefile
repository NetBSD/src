#	$NetBSD: Makefile,v 1.7 2011/05/26 12:56:34 joerg Exp $

WARNS?=	1	# XXX -Wsign-compare -Wshadow

USE_FORT?= yes	# network server

PROG=	ndbootd
SRCS=	ndbootd.c
MAN=	ndbootd.8

CPPFLAGS+=-DHAVE_CONFIG_H -I${.CURDIR}

.if ${MACHINE_ARCH} == "m68000"
COPTS.ndbootd.c+=	-fno-tree-ter -fno-tree-lrs
.endif

CWARNFLAGS.clang+=	-Wno-tautological-compare

.include <bsd.own.mk>
.include <bsd.prog.mk>
