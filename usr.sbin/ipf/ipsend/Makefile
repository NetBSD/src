#	$NetBSD: Makefile,v 1.4 1997/04/13 14:51:55 mrg Exp $

PROG=	ipsend
SRCS=	ipsend.c ip.c ipsopt.c 44arp.c sbpf.c
NOMAN=	yes
CFLAGS+=-I${.CURDIR}/../ipftest -I${.CURDIR}/../ipf

.PATH: ${.CURDIR}/../ipf
.include <bsd.prog.mk>
