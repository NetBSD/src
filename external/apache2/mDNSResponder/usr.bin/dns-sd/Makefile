#	$NetBSD: Makefile,v 1.2 2018/01/14 20:16:04 christos Exp $

PROG=	dns-sd

.include <bsd.init.mk>

.PATH:	${DIST}/Clients ${DIST}/mDNSShared

SRCS=	dns-sd.c ClientCommon.c
CPPFLAGS+=-I${DIST}/mDNSShared
LDADD+= -ldns_sd
DPADD+= ${LIBDNS_SD}

.include <bsd.prog.mk>
