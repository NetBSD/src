#	$NetBSD: Makefile,v 1.6 1996/12/13 08:25:56 mikel Exp $

LIB=	pcap
MAN=	pcap.3

LEX=	flex

CFLAGS+=-I. -I${.CURDIR} -DYYBISON
CFLAGS+=-DHAVE_MALLOC_H=1 -DHAVE_SYS_IOCCOM_H=1 -DHAVE_SYS_SOCKIO_H=1
CFLAGS+=-DHAVE_ETHER_HOSTTON=1 -DHAVE_STRERROR=1 -DHAVE_SOCKADDR_SA_LEN=1
CFLAGS+=-DLBL_ALIGN=1 
LFLAGS+=-olex.yy.c -Ppcap_
YFLAGS+=-ppcap_

HDRS=	pcap.h pcap-namedb.h
SRCS=	pcap.c inet.c gencode.c optimize.c nametoaddr.c etherent.c \
	savefile.c bpf_filter.c bpf_image.c grammar.y scanner.l \
	pcap-bpf.c
.PATH:	${.CURDIR}/../../sys/net

CLEANFILES+= grammar.c scanner.c y.tab.h

includes:
	@cd ${.CURDIR}; for i in $(HDRS); do \
	    j="cmp -s $$i ${DESTDIR}/usr/include/$$i || \
		${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 $$i \
		${DESTDIR}/usr/include"; \
	    echo $$j; \
	    eval "$$j"; \
	done

.include <bsd.lib.mk>

.depend: grammar.c scanner.c
