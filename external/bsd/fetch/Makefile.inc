# $NetBSD: Makefile.inc,v 1.1.4.2 2008/10/19 22:39:50 haad Exp $

.include <bsd.own.mk>

LIBFETCHDIR=	${NETBSDSRCDIR}/external/bsd/fetch/dist/libfetch

USE_FORT?=	yes # complex string handling

WARNS?=	4
