#	$NetBSD: Makefile,v 1.2 1997/03/29 04:28:56 darrenr Exp $

PROG=	iptest
SRCS=	iptest.c iptests.c ip.c sbpf.c 44arp.c sock.c
NOMAN=	yes
CFLAGS+=	-I${.CURDIR}/../ipsend -I${.CURDIR}/../ipftest -I${.CURDIR}/../../../sbin/ipf -I${.CURDIR}/../../../sys

.PATH: ${.CURDIR}/../../../sbin/ipf ${.CURDIR}/../ipsend
.include <bsd.prog.mk>
