#	$NetBSD: Makefile,v 1.1.1.1 1997/10/03 15:38:56 christos Exp $

WARNS?=	1
LIB=	pcap
MAN=	pcap.3 

LEX=	flex

CFLAGS+=-I. -I${.CURDIR} -DYYBISON
CFLAGS+=-DHAVE_MALLOC_H=1 -DHAVE_SYS_IOCCOM_H=1 -DHAVE_SYS_SOCKIO_H=1
CFLAGS+=-DHAVE_ETHER_HOSTTON=1 -DHAVE_STRERROR=1 -DHAVE_SOCKADDR_SA_LEN=1
CFLAGS+=-DLBL_ALIGN=1
LFLAGS+=-olex.yy.c -Ppcap_
YFLAGS+=-ppcap_

SRCS=	scanner.l savefile.c pcap.c pcap-bpf.c optimize.c nametoaddr.c \
inet.c grammar.y gencode.c etherent.c bpf_image.c 
SRCS+=	bpf_filter.c

.PATH:	${.CURDIR}/../../sys/net

INCS=	pcap-namedb.h pcap.h 
INCSDIR=/usr/include

CLEANFILES+= y.tab.h

.include <bsd.lib.mk>
