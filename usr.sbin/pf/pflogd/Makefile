#	$NetBSD: Makefile,v 1.5.6.1 2012/04/17 00:09:51 yamt Exp $
#	$OpenBSD: Makefile,v 1.6 2003/11/20 23:23:09 avsm Exp $

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

WARNS ?= 4
CPPFLAGS+=-I${NETBSDSRCDIR}/dist/pf/sbin/pflogd
CPPFLAGS+=-I${NETBSDSRCDIR}/external/bsd/libpcap/dist
CPPFLAGS+=-DPCAP_DONT_INCLUDE_PCAP_BPF_H


LDADD+= -lpcap -lutil
DPAPP+=	${LIBPCAP} ${LIBUTIL}

PROG=	pflogd
SRCS=	pflogd.c privsep.c privsep_fdpass.c
MAN=	pflogd.8

COPTS.privsep_fdpass.c = -Wno-stack-protector

BINDIR=		/sbin
.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif

.include <bsd.prog.mk>

.PATH: ${NETBSDSRCDIR}/dist/pf/sbin/pflogd
