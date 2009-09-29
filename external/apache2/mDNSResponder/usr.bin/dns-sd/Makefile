#	$NetBSD: Makefile,v 1.1 2009/09/29 23:56:34 tsarna Exp $

PROG=	dns-sd

.include <bsd.init.mk>

.PATH:	${DIST}/Clients ${DIST}/mDNSShared

SRCS=	dns-sd.c ClientCommon.c
LDADD+= -ldns_sd
DPADD+= ${LIBDNS_SD}

.include <bsd.prog.mk>
