#	$NetBSD: Makefile,v 1.3 1997/03/29 20:19:48 thorpej Exp $

PROG=	ipsend
SRCS=	ipsend.c ip.c ipsopt.c 44arp.c sbpf.c
NOMAN=	yes
CFLAGS+=-I${.CURDIR}/../ipftest -I${.CURDIR}/../../../sbin/ipf

.PATH: ${.CURDIR}/../../../sbin/ipf
.include <bsd.prog.mk>
