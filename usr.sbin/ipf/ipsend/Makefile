#	$NetBSD: Makefile,v 1.1.1.1 1997/01/05 13:09:05 mrg Exp $

PROG=	ipsend
SRCS=	ipsend.c ip.c ipsopt.c 44arp.c sbpf.c
NOMAN=	yes
CFLAGS+=	-I${.CURDIR}/../../../sbin/ipf -I${.CURDIR}/../ipftest

.PATH: ${.CURDIR}/../../../sbin/ipf
.include <bsd.prog.mk>
