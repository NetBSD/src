#	$NetBSD: Makefile,v 1.7 1997/05/08 21:12:18 gwr Exp $

PROG=	ipnat
SRCS=	kmem.c ipnat.c
MAN=	ipnat.8 ipnat.4 ipnat.5
CFLAGS+=-I${.CURDIR}/../ipfstat -I${.CURDIR}/../ipf

.PATH: ${.CURDIR}/../ipf ${.CURDIR}/../ipfstat
.include <bsd.prog.mk>
