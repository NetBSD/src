#	$NetBSD: Makefile,v 1.4 2005/02/20 11:12:43 cube Exp $

NOLINT=	# defined

.include <bsd.own.mk>

.PATH: ${NETBSDSRCDIR}/dist/pppd/pppd/plugins

LIB=minconn
SRCS=minconn.c
CPPFLAGS+=-I${NETBSDSRCDIR}/dist/pppd/pppd

LIBPREFIX=
SHLIB_FULLVERSION=

.include <bsd.lib.mk>
