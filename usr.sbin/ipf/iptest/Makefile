#	$NetBSD: Makefile,v 1.1.1.1 1997/01/05 13:09:05 mrg Exp $

PROG=	iptest
SRCS=	iptest.c iptests.c ip.c sbpf.c 44arp.c sock.c
NOMAN=	yes
CFLAGS+=	-I${.CURDIR}/../../../sbin/ipf -I${.CURDIR}/../ipsend -I${.CURDIR}/../ipftest

.PATH: ${.CURDIR}/../../../sbin/ipf ${.CURDIR}/../ipsend
.include <bsd.prog.mk>
