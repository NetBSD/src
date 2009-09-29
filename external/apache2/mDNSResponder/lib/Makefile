#	$NetBSD: Makefile,v 1.1 2009/09/29 23:56:28 tsarna Exp $

.include <bsd.init.mk>

LIB=	dns_sd

SRCS=	dnssd_clientlib.c dnssd_clientstub.c dnssd_ipc.c  
CPPFLAGS+=-I${DIST}/mDNSShared

INCS=	dns_sd.h
INCSDIR=/usr/include

.PATH:	${DIST}/mDNSShared

zoot:
	ls ${DIST}/mDNSShared/dnssd_clientlib.c
	
.include <bsd.lib.mk>
