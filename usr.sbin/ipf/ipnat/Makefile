#	$NetBSD: Makefile,v 1.6 1997/05/06 20:46:10 gwr Exp $

PROG=	ipnat
SRCS=	kmem.c ipnat.c
MAN=	ipnat.8 ipnat.4 ipnat.5
CFLAGS+=-I${.CURDIR}/../ipfstat -I${.CURDIR}/../ipf

.PATH.c: ${.CURDIR}/../ipf ${.CURDIR}/../ipfstat
.include <bsd.prog.mk>
