#	$NetBSD: Makefile,v 1.1 2005/02/20 11:14:37 cube Exp $

NOLINT=	# defined

.include <bsd.own.mk>

.PATH: ${NETBSDSRCDIR}/dist/pppd/pppd/plugins

LIB=passwordfd
SRCS=passwordfd.c
CPPFLAGS+=-I${NETBSDSRCDIR}/dist/pppd/pppd

LIBPREFIX=
SHLIB_FULLVERSION=

.include <bsd.lib.mk>
