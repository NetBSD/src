#	$NetBSD: Makefile,v 1.1.1.2 1997/05/25 11:46:48 darrenr Exp $

PROG=	ipsend
SRCS=	ipsend.c ip.c ipsopt.c 44arp.c sbpf.c
NOMAN=	yes
CFLAGS+=-I${.CURDIR}/../ipftest -I${.CURDIR}/../ipf

.PATH: ${.CURDIR}/../ipf
.include <bsd.prog.mk>
