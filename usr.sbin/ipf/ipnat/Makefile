#	$NetBSD: Makefile,v 1.3 1997/03/29 19:49:06 thorpej Exp $

PROG=	ipnat
SRCS=	kmem.c ipnat.c
MAN=	ipnat.8 ipnat.4 ipnat.5
CFLAGS+=-I${.CURDIR}/../ipfstat -I${.CURDIR}/../../../sbin/ipf -I${.CURDIR}/../../../sys

.PATH: ${.CURDIR}/../../../sbin/ipf ${.CURDIR}/../ipfstat
.include <bsd.prog.mk>
