#	$NetBSD: Makefile,v 1.1.42.1 2014/05/22 11:43:05 yamt Exp $

.include <bsd.own.mk>

PROG=	lmcconfig
MAN=	lmcconfig.8
CPPFLAGS+=-I${NETBSDSRCDIR}/sys/dev/pci

.if ${MACHINE_ARCH} == "vax"
COPTS.lmcconfig.c=-O0
.endif

.include <bsd.prog.mk>
