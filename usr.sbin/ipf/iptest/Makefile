#	$NetBSD: Makefile,v 1.7 1997/05/08 21:12:22 gwr Exp $

PROG=	iptest
SRCS=	iptest.c iptests.c ip.c sbpf.c 44arp.c sock.c
NOMAN=	yes
CFLAGS+=-I${.CURDIR}/../ipsend -I${.CURDIR}/../ipftest -I${.CURDIR}/../ipf

.PATH: ${.CURDIR}/../ipf ${.CURDIR}/../ipsend
.include <bsd.prog.mk>
