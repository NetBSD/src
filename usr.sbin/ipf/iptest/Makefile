#	$NetBSD: Makefile,v 1.11 1997/10/25 06:58:17 lukem Exp $

PROG=	iptest
SRCS=	iptest.c iptests.c ip.c sbpf.c 44arp.c sock.c
MAN=	iptest.1
CPPFLAGS+=-I${.CURDIR}/../ipsend -I${.CURDIR}/../ipftest -I${.CURDIR}/../ipf

.PATH: ${.CURDIR}/../ipf ${.CURDIR}/../ipsend
.include <bsd.prog.mk>
