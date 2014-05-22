#	$NetBSD: Makefile,v 1.1.4.2 2014/05/22 15:51:09 yamt Exp $

NOLINT=	# defined

.include <bsd.own.mk>

.PATH: ${NETBSDSRCDIR}/dist/pppd/pppd/plugins

LIB=minconn
SRCS=minconn.c
CPPFLAGS+=-I${NETBSDSRCDIR}/dist/pppd/pppd

LIBPREFIX=
SHLIB_FULLVERSION=

.include <bsd.lib.mk>
