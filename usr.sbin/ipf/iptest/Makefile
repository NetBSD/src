#	$NetBSD: Makefile,v 1.14 2002/09/18 03:54:31 lukem Exp $

.include <bsd.own.mk>

PROG=	iptest
SRCS=	iptest.c iptests.c ip.c sbpf.c sock.c 44arp.c
MAN=	iptest.1
CPPFLAGS+=-I${NETBSDSRCDIR}/dist/ipf

.PATH:	${NETBSDSRCDIR}/dist/ipf/ipsend
.include <bsd.prog.mk>
