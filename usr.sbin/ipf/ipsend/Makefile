#	$NetBSD: Makefile,v 1.5 1997/05/06 20:46:12 gwr Exp $

PROG=	ipsend
SRCS=	ipsend.c ip.c ipsopt.c 44arp.c sbpf.c
NOMAN=	yes
CFLAGS+=-I${.CURDIR}/../ipftest -I${.CURDIR}/../ipf

.PATH.c: ${.CURDIR}/../ipf
.include <bsd.prog.mk>
