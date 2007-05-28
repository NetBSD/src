# $NetBSD: Makefile,v 1.30 2007/05/28 12:06:20 tls Exp $	

.include <bsd.own.mk>

USE_FORT?= yes	# network protocol library

SRCDIR=		${NETBSDSRCDIR}/dist/libpcap
.PATH:		${SRCDIR}

LIB=		pcap
MAN=		pcap.3

WARNS?=		3

CPPFLAGS+=	-I${.CURDIR} -I. -I${SRCDIR}
CPPFLAGS+=	-DYYBISON
LPREFIX=	pcap_
YPREFIX=	pcap_
YHEADER=	1

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-D_U_="__attribute__((unused))"

.if (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
.endif

SRCS=	scanner.l savefile.c pcap.c pcap-bpf.c optimize.c nametoaddr.c \
	inet.c grammar.y gencode.c fad-getad.c etherent.c bpf_image.c \
	bpf_dump.c version.c

.PATH:	${NETBSDSRCDIR}/sys/net
SRCS+=	bpf_filter.c

INCS=		pcap-namedb.h pcap.h
INCSDIR=	/usr/include

scanner.d scanner.o: tokdefs.h

tokdefs.h: grammar.h
	cp $? $@

CLEANFILES+=	tokdefs.h

.include <bsd.lib.mk>
