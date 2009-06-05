# $NetBSD: Makefile.inc,v 1.1.14.2 2009/06/05 17:19:28 snj Exp $

.include <bsd.own.mk>

LIBFETCHDIR=	${NETBSDSRCDIR}/external/bsd/fetch/dist/libfetch

USE_FORT?=	yes # complex string handling

WARNS?=	4
