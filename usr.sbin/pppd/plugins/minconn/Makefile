#	$NetBSD: Makefile,v 1.3 2002/09/18 13:31:55 lukem Exp $

NOLINT=	# defined

.include <bsd.own.mk>

PCAPDIR=${NETBSDSRCDIR}/lib/libpcap

LIB=minconn
SRCS=minconn.c
CPPFLAGS+=-I${.CURDIR}/../pppd

LIBPREFIX=
SHLIB_FULLVERSION=

.include <bsd.lib.mk>
