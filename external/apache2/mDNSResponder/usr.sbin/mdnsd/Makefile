#	$NetBSD: Makefile,v 1.9.2.1 2018/04/16 01:57:33 pgoyette Exp $

PROG=	mdnsd

.include <bsd.init.mk>

.PATH:	${DIST}/mDNSPosix ${DIST}/mDNSCore ${DIST}/mDNSShared

CPPFLAGS+=-I${DIST}/mDNSCore -I${DIST}/mDNSShared \
	-DPROGRAM_NAME=\"mdnsd\" -DHAVE_SIGINFO
#CPPFLAGS+=-DMDNS_DEBUGMSGS=99
SRCS=	PosixDaemon.c mDNSPosix.c mDNSUNP.c mDNS.c DNSDigest.c uDNS.c \
	DNSCommon.c uds_daemon.c mDNSDebug.c dnssd_ipc.c GenLinkedList.c \
	PlatformCommon.c CryptoAlg.c anonymous.c

COPTS.uds_daemon.c += -Wno-stack-protector

MAN=	mdnsd.8

CWARNFLAGS.clang+=	-Wno-unused-value -Wno-error=address-of-packed-member

.include <bsd.prog.mk>
