#	$NetBSD: Makefile,v 1.11.2.1 1999/12/20 21:06:56 he Exp $

PROG=	iptest
SRCS=	iptest.c iptests.c ip.c sbpf.c 44arp.c sock.c
MAN=	iptest.1
CPPFLAGS+=-I${.CURDIR}/../../../dist/ipf

.PATH:	${.CURDIR}/../../../dist/ipf/ipsend
.include <bsd.prog.mk>
