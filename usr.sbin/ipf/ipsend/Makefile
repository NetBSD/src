#	$NetBSD: Makefile,v 1.6 1997/05/08 21:12:20 gwr Exp $

PROG=	ipsend
SRCS=	ipsend.c ip.c ipsopt.c 44arp.c sbpf.c
NOMAN=	yes
CFLAGS+=-I${.CURDIR}/../ipftest -I${.CURDIR}/../ipf

.PATH: ${.CURDIR}/../ipf
.include <bsd.prog.mk>
