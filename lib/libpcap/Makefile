#	$NetBSD: Makefile,v 1.4 1996/08/10 22:31:19 explorer Exp $

LIB=	pcap
MAN=	pcap.3

CFLAGS+=-I. -I${.CURDIR} -DFDDI

HDRS=	pcap.h pcap-namedb.h
SRCS=	pcap.c inet.c gencode.c optimize.c nametoaddr.c etherent.c \
	savefile.c bpf_filter.c bpf_image.c grammar.y scanner.l \
	pcap-bpf.c
.PATH:	${.CURDIR}/../../sys/net

CLEANFILES+= grammar.c scanner.c y.tab.h

includes:
	@cd ${.CURDIR}; for i in $(HDRS); do \
	    j="cmp -s $$i ${DESTDIR}/usr/include/$$i || \
		install -c -o ${BINOWN} -g ${BINGRP} -m 444 $$i \
		${DESTDIR}/usr/include"; \
	    echo $$j; \
	    eval "$$j"; \
	done

.include <bsd.lib.mk>
