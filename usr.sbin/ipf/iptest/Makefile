#	$NetBSD: Makefile,v 1.11.6.1 1999/12/27 18:37:48 wrstuden Exp $

PROG=	iptest
SRCS=	iptest.c iptests.c ip.c sbpf.c 44arp.c sock.c
MAN=	iptest.1
CPPFLAGS+=-I${.CURDIR}/../../../dist/ipf

.PATH:	${.CURDIR}/../../../dist/ipf/ipsend
.include <bsd.prog.mk>
