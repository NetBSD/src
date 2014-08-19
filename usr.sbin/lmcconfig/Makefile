#	$NetBSD: Makefile,v 1.1.48.1 2014/08/20 00:05:09 tls Exp $

.include <bsd.own.mk>

PROG=	lmcconfig
MAN=	lmcconfig.8
CPPFLAGS+=-I${NETBSDSRCDIR}/sys/dev/pci

.if ${MACHINE_ARCH} == "vax"
COPTS.lmcconfig.c=-O0
.endif

.include <bsd.prog.mk>
