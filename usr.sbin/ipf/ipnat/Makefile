#	$NetBSD: Makefile,v 1.4 1997/03/29 20:09:16 thorpej Exp $

PROG=	ipnat
SRCS=	kmem.c ipnat.c
MAN=	ipnat.8 ipnat.4 ipnat.5
CFLAGS+=-I${.CURDIR}/../ipfstat -I${.CURDIR}/../../../sbin/ipf

.PATH: ${.CURDIR}/../../../sbin/ipf ${.CURDIR}/../ipfstat
.include <bsd.prog.mk>
