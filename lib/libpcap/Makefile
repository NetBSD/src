#	$NetBSD: Makefile,v 1.14.2.1 1997/11/09 01:00:39 lukem Exp $	

LIB=	pcap
MAN=	pcap.3 

LEX=	flex

CPPFLAGS+=-I. -I${.CURDIR} -DYYBISON
CPPFLAGS+=-DHAVE_MALLOC_H=1 -DHAVE_SYS_IOCCOM_H=1 -DHAVE_SYS_SOCKIO_H=1
CPPFLAGS+=-DHAVE_ETHER_HOSTTON=1 -DHAVE_STRERROR=1 -DHAVE_SOCKADDR_SA_LEN=1
CFPPLAGS+=-DLBL_ALIGN=1
LFLAGS+=-olex.yy.c -Ppcap_
YFLAGS+=-ppcap_

SRCS=	scanner.l savefile.c pcap.c pcap-bpf.c optimize.c nametoaddr.c \
inet.c grammar.y gencode.c etherent.c bpf_image.c 
SRCS+=	bpf_filter.c version.c

.PATH:	${.CURDIR}/../../sys/net

INCS=	pcap-namedb.h pcap.h 
INCSDIR=/usr/include

CLEANFILES+= y.tab.h

scanner.c:	y.tab.h

y.tab.c y.tab.h: grammar.c

.include <bsd.lib.mk>
