#	$NetBSD: Makefile,v 1.2.60.1 2025/08/02 05:20:50 perseant Exp $

.include <bsd.init.mk>

LIB=	dns_sd

SRCS=	dnssd_clientlib.c dnssd_clientstub.c dnssd_ipc.c  
CPPFLAGS+=-I${DIST}/mDNSShared -I${DIST}/mDNSCore -DmDNSResponderVersion=2559

COPTS.dnssd_clientstub.c += -Wno-stack-protector

INCS=	dns_sd.h
INCSDIR=/usr/include

.PATH:	${DIST}/mDNSShared

.include <bsd.lib.mk>
