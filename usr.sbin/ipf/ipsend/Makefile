#	$NetBSD: Makefile,v 1.2 1997/03/29 04:28:52 darrenr Exp $

PROG=	ipsend
SRCS=	ipsend.c ip.c ipsopt.c 44arp.c sbpf.c
NOMAN=	yes
CFLAGS+=	-I${.CURDIR}/../ipftest -I${.CURDIR}/../../../sbin/ipf -I${.CURDIR}/../../../sys

.PATH: ${.CURDIR}/../../../sbin/ipf
.include <bsd.prog.mk>
