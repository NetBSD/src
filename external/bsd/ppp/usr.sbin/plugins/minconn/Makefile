#	$NetBSD: Makefile,v 1.1 2013/11/28 22:33:43 christos Exp $

NOLINT=	# defined

.include <bsd.own.mk>

.PATH: ${NETBSDSRCDIR}/dist/pppd/pppd/plugins

LIB=minconn
SRCS=minconn.c
CPPFLAGS+=-I${NETBSDSRCDIR}/dist/pppd/pppd

LIBPREFIX=
SHLIB_FULLVERSION=

.include <bsd.lib.mk>
