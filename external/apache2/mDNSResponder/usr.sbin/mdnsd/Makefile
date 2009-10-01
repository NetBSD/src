#	$NetBSD: Makefile,v 1.3 2009/10/01 10:01:14 tron Exp $

PROG=	mdnsd

.include <bsd.init.mk>

.PATH:	${DIST}/mDNSPosix ${DIST}/mDNSCore ${DIST}/mDNSShared

CPPFLAGS+=-I${DIST}/mDNSCore -I${DIST}/mDNSShared -DPROGRAM_NAME=\"mdnsd\"
SRCS=	PosixDaemon.c mDNSPosix.c mDNSUNP.c mDNS.c DNSDigest.c uDNS.c \
	DNSCommon.c uds_daemon.c mDNSDebug.c dnssd_ipc.c GenLinkedList.c \
	PlatformCommon.c

COPTS.uds_daemon.c += -Wno-stack-protector

MAN=	mdnsd.8

mdnsd.8: mDNSResponder.8
	${TOOL_CAT} < $? > $@
                                
.include <bsd.prog.mk>
