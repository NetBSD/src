#	$NetBSD: Makefile,v 1.24 2003/08/01 17:03:59 lukem Exp $

.include <bsd.own.mk>

LIB=	pcap
MAN=	pcap.3

WARNS?=	1

CPPFLAGS+=-I. -I${.CURDIR} -DYYBISON
CPPFLAGS+=-DINET6
CPPFLAGS+=-DHAVE_MALLOC_H=1 -DHAVE_SYS_IOCCOM_H=1 -DHAVE_SYS_SOCKIO_H=1
CPPFLAGS+=-DHAVE_ETHER_HOSTTON=1 -DHAVE_STRERROR=1 -DHAVE_SOCKADDR_SA_LEN=1
CPPFLAGS+=-DHAVE_IFADDRS_H=1
# used in no place
#CPPFLAGS+=-DLBL_ALIGN=1
LPREFIX=pcap_
YPREFIX=pcap_
YHEADER=1

SRCS=	scanner.l savefile.c pcap.c pcap-bpf.c optimize.c nametoaddr.c \
	inet.c grammar.y gencode.c etherent.c bpf_image.c
SRCS+=	bpf_filter.c version.c

.PATH:	${NETBSDSRCDIR}/sys/net

INCS=	pcap-namedb.h pcap.h
INCSDIR=/usr/include

.include <bsd.lib.mk>
