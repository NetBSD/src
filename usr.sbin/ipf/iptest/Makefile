#	$NetBSD: Makefile,v 1.1.1.8 1998/05/17 16:29:57 veego Exp $

PROG=	iptest
SRCS=	iptest.c iptests.c ip.c sbpf.c 44arp.c sock.c
MAN=	iptest.1
CPPFLAGS+=-I${.CURDIR}/../ipsend -I${.CURDIR}/../ipftest -I${.CURDIR}/../ipf

.PATH: ${.CURDIR}/../ipf ${.CURDIR}/../ipsend
.include <bsd.prog.mk>
