#	$NetBSD: Makefile,v 1.14 2021/04/13 06:25:48 mrg Exp $

PROG=	mdnsd

.include <bsd.init.mk>

.PATH:	${DIST}/mDNSPosix ${DIST}/mDNSCore ${DIST}/mDNSShared

CPPFLAGS+=-I${DIST}/mDNSCore -I${DIST}/mDNSShared \
	-DPROGRAM_NAME=\"mdnsd\" -DHAVE_SIGINFO
#CPPFLAGS+=-DMDNS_DEBUGMSGS=99
SRCS=	PosixDaemon.c mDNSPosix.c mDNSBSD.c mDNS.c DNSDigest.c uDNS.c \
	DNSCommon.c uds_daemon.c mDNSDebug.c dnssd_ipc.c GenLinkedList.c \
	PlatformCommon.c CryptoAlg.c anonymous.c

COPTS.uds_daemon.c += -Wno-stack-protector

MAN=	mdnsd.8

CWARNFLAGS.clang+=	-Wno-unused-value -Wno-error=address-of-packed-member
CWARNFLAGS.gcc+=	${GCC_NO_ADDR_OF_PACKED_MEMBER}

COPTS.DNSCommon.c+=	${GCC_NO_RETURN_LOCAL_ADDR}

.include <bsd.prog.mk>
