#	$NetBSD: Makefile,v 1.4 1997/03/29 20:23:31 thorpej Exp $

PROG=	iptest
SRCS=	iptest.c iptests.c ip.c sbpf.c 44arp.c sock.c
NOMAN=	yes
CFLAGS+=-I${.CURDIR}/../ipsend -I${.CURDIR}/../ipftest -I${.CURDIR}/../../../sbin/ipf

.PATH: ${.CURDIR}/../../../sbin/ipf ${.CURDIR}/../ipsend
.include <bsd.prog.mk>
