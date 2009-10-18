#	$NetBSD: Makefile,v 1.5 2009/10/18 17:43:11 tron Exp $

PROG=	mdnsd

.include <bsd.init.mk>

.PATH:	${DIST}/mDNSPosix ${DIST}/mDNSCore ${DIST}/mDNSShared

CPPFLAGS+=-I${DIST}/mDNSCore -I${DIST}/mDNSShared \
	-DPROGRAM_NAME=\"mdnsd\" -DHAVE_SIGINFO
SRCS=	PosixDaemon.c mDNSPosix.c mDNSUNP.c mDNS.c DNSDigest.c uDNS.c \
	DNSCommon.c uds_daemon.c mDNSDebug.c dnssd_ipc.c GenLinkedList.c \
	PlatformCommon.c

COPTS.uds_daemon.c += -Wno-stack-protector

MAN=	mdnsd.8

mdnsd.8: mDNSResponder.8
	${TOOL_CAT} < $? > $@

CLEANFILES+=	mdnsd.8
                               
.include <bsd.prog.mk>
