#	$NetBSD: Makefile,v 1.10 1997/05/09 13:59:18 mycroft Exp $

LIB=	pcap
MAN=	pcap.3

LEX=	flex

CFLAGS+=-I. -I${.CURDIR} -DYYBISON
CFLAGS+=-DHAVE_MALLOC_H=1 -DHAVE_SYS_IOCCOM_H=1 -DHAVE_SYS_SOCKIO_H=1
CFLAGS+=-DHAVE_ETHER_HOSTTON=1 -DHAVE_STRERROR=1 -DHAVE_SOCKADDR_SA_LEN=1
CFLAGS+=-DLBL_ALIGN=1
LFLAGS+=-olex.yy.c -Ppcap_
YFLAGS+=-ppcap_

SRCS=	bpf_filter.c bpf_image.c etherent.c gencode.c grammar.y inet.c \
	nametoaddr.c optimize.c pcap-bpf.c pcap.c savefile.c scanner.l

.PATH:	${.CURDIR}/../../sys/net

INCS=pcap.h pcap-namedb.h
INCSDIR=/usr/include

.include <bsd.lib.mk>
