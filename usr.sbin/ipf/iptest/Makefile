#	$NetBSD: Makefile,v 1.15 2004/03/28 09:01:28 martti Exp $

.include <bsd.own.mk>

PROG=		iptest
SRCS=		iptest.c iptests.c ip.c sbpf.c sock.c 44arp.c
MAN=		iptest.1

CPPFLAGS+=	-I${NETBSDSRCDIR}/dist/ipf

.PATH:		${NETBSDSRCDIR}/dist/ipf/ipsend

.include <bsd.prog.mk>
