#	$NetBSD: Makefile,v 1.2 2023/11/15 22:11:16 christos Exp $

NOLINT=	# defined

.include <bsd.own.mk>

.PATH: ${PPPD}/plugins

LIB=minconn
SRCS=minconn.c
CPPFLAGS+=-I${PPPD}

LIBPREFIX=
SHLIB_FULLVERSION=

.include <bsd.lib.mk>
