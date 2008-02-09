#	$NetBSD: Makefile,v 1.4 2008/02/09 02:37:22 mrg Exp $

USE_FORT?= yes	# network server

PROG=	ndbootd
SRCS=	ndbootd.c
MAN=	ndbootd.8

CPPFLAGS+=-DHAVE_CONFIG_H -I${.CURDIR}

.include <bsd.own.mk>
.include <bsd.prog.mk>
