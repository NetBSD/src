#	$NetBSD: Makefile,v 1.2 2012/03/24 02:08:34 christos Exp $

.include <bsd.own.mk>
.include "../Makefile.inc"

PROG=		iptest
SRCS=		iptest.c iptests.c ip.c sbpf.c sock.c 44arp.c
MAN=		iptest.1

CPPFLAGS+=	-I${.CURDIR}/../ipsend -I${NETBSDSRCDIR}/sys
.PATH:		${UDIST}/ipsend

.include <bsd.prog.mk>
