#	$NetBSD: Makefile,v 1.8 1997/05/25 12:05:31 darrenr Exp $

PROG=	iptest
SRCS=	iptest.c iptests.c ip.c sbpf.c 44arp.c sock.c
NOMAN=	yes
CFLAGS+=-I${.CURDIR}/../ipsend -I${.CURDIR}/../ipftest -I${.CURDIR}/../ipf

.PATH: ${.CURDIR}/../ipf ${.CURDIR}/../ipsend
.include <bsd.prog.mk>
