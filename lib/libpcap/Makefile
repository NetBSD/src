#	$NetBSD: Makefile,v 1.18 1999/07/02 10:05:22 itojun Exp $	

LIB=	pcap
MAN=	pcap.3 

LEX=	flex

CPPFLAGS+=-I. -I${.CURDIR} -DYYBISON
CPPFLAGS+=-DINET6
CPPFLAGS+=-DHAVE_MALLOC_H=1 -DHAVE_SYS_IOCCOM_H=1 -DHAVE_SYS_SOCKIO_H=1
CPPFLAGS+=-DHAVE_ETHER_HOSTTON=1 -DHAVE_STRERROR=1 -DHAVE_SOCKADDR_SA_LEN=1
CFPPLAGS+=-DLBL_ALIGN=1
LPREFIX=pcap_
YPREFIX=pcap_
YHEADER=1

SRCS=	scanner.l savefile.c pcap.c pcap-bpf.c optimize.c nametoaddr.c \
	inet.c grammar.y gencode.c etherent.c bpf_image.c 
SRCS+=	bpf_filter.c version.c

.PATH:	${.CURDIR}/../../sys/net

INCS=	pcap-namedb.h pcap.h 
INCSDIR=/usr/include

scanner.c: grammar.h

.include <bsd.lib.mk>
