#	$NetBSD: Makefile,v 1.1.1.2 2004/09/19 21:31:46 dyoung Exp $	

LIB=	pcap
MAN=	pcap.3 

LEX=	flex

CFLAGS+=-I. -I${.CURDIR} -DYYBISON
CFLAGS+=-DHAVE_MALLOC_H=1 -DHAVE_SYS_IOCCOM_H=1 -DHAVE_SYS_SOCKIO_H=1
CFLAGS+=-DHAVE_ETHER_HOSTTON=1 -DHAVE_STRERROR=1 -DHAVE_SOCKADDR_SA_LEN=1
CFLAGS+=-DLBL_ALIGN=1
LFLAGS+=-olex.yy.c -Ppcap_
YFLAGS+=-ppcap_
YHEADER=1

SRCS=	scanner.l savefile.c pcap.c pcap-bpf.c optimize.c nametoaddr.c \
inet.c grammar.y gencode.c fad-getad.c etherent.c bpf_image.c \
bpf_dump.c 
SRCS+=	bpf_filter.c version.c

.PATH:	${.CURDIR}/../../sys/net

INCS=	pcap-namedb.h pcap.h 
INCSDIR=/usr/include

scanner.c: grammar.h

.include <bsd.lib.mk>
