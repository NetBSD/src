# $NetBSD: Makefile.inc,v 1.1.12.2 2009/06/05 17:01:45 snj Exp $

.include <bsd.own.mk>

LIBFETCHDIR=	${NETBSDSRCDIR}/external/bsd/fetch/dist/libfetch

USE_FORT?=	yes # complex string handling

WARNS?=	4
