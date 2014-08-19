#	$NetBSD: Makefile,v 1.1.8.2 2014/08/19 23:52:12 tls Exp $

NOLINT=	# defined

.include <bsd.own.mk>

.PATH: ${NETBSDSRCDIR}/dist/pppd/pppd/plugins

LIB=minconn
SRCS=minconn.c
CPPFLAGS+=-I${NETBSDSRCDIR}/dist/pppd/pppd

LIBPREFIX=
SHLIB_FULLVERSION=

.include <bsd.lib.mk>
