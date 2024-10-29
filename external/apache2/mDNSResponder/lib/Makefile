#	$NetBSD: Makefile,v 1.4 2024/10/29 18:41:57 christos Exp $

.include <bsd.init.mk>

LIB=	dns_sd

SRCS=	dnssd_clientlib.c dnssd_clientstub.c dnssd_ipc.c  
CPPFLAGS+=-I${DIST}/mDNSShared -I${DIST}/mDNSCore -DmDNSResponderVersion=2559

COPTS.dnssd_clientstub.c += -Wno-stack-protector

INCS=	dns_sd.h
INCSDIR=/usr/include

.PATH:	${DIST}/mDNSShared

.include <bsd.lib.mk>
