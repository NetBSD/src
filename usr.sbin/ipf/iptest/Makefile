#	$NetBSD: Makefile,v 1.10 1997/09/21 18:02:19 veego Exp $

PROG=	iptest
SRCS=	iptest.c iptests.c ip.c sbpf.c 44arp.c sock.c
MAN=	iptest.1
CFLAGS+=-I${.CURDIR}/../ipsend -I${.CURDIR}/../ipftest -I${.CURDIR}/../ipf

.PATH: ${.CURDIR}/../ipf ${.CURDIR}/../ipsend
.include <bsd.prog.mk>
