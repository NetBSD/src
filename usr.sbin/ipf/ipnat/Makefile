#	$NetBSD: Makefile,v 1.1.1.6 1997/09/21 16:49:03 veego Exp $

PROG=	ipnat
SRCS=	kmem.c ipnat.c
MAN=	ipnat.8 ipnat.4 ipnat.5
MLINKS=	ipnat.5 ipnat.conf.5
CFLAGS+=-I${.CURDIR}/../ipf -I${.CURDIR}/../ipfstat

.PATH: ${.CURDIR}/../ipf ${.CURDIR}/../ipfstat
.include <bsd.prog.mk>
