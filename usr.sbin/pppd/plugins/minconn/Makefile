#	$NetBSD: Makefile,v 1.2 2002/09/18 03:54:34 lukem Exp $

.include <bsd.own.mk>

PCAPDIR=${NETBSDSRCDIR}/lib/libpcap

NOLINT=yes
LIBPREFIX=
SHLIB_FULLVERSION=
LIB=minconn
SRCS=minconn.c
CPPFLAGS+=-I${.CURDIR}/../pppd

.include <bsd.lib.mk>
