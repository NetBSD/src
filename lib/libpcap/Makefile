#	$NetBSD: Makefile,v 1.8 1997/04/05 08:12:16 veego Exp $

LIB=	pcap
MAN=	pcap.3

LEX=	flex

CFLAGS+=-I. -I${.CURDIR} -DYYBISON
CFLAGS+=-DHAVE_MALLOC_H=1 -DHAVE_SYS_IOCCOM_H=1 -DHAVE_SYS_SOCKIO_H=1
CFLAGS+=-DHAVE_ETHER_HOSTTON=1 -DHAVE_STRERROR=1 -DHAVE_SOCKADDR_SA_LEN=1
CFLAGS+=-DLBL_ALIGN=1
LFLAGS+=-olex.yy.c -Ppcap_
YFLAGS+=-ppcap_

SRCS=	pcap.c inet.c gencode.c optimize.c nametoaddr.c etherent.c \
	savefile.c bpf_filter.c bpf_image.c grammar.y scanner.l \
	pcap-bpf.c
.PATH:	${.CURDIR}/../../sys/net

INCS=pcap.h pcap-namedb.h
INCSDIR=/usr/include

CLEANFILES+= grammar.c scanner.c y.tab.h

.include <bsd.lib.mk>

.depend: grammar.c scanner.c
