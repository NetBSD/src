#	$NetBSD: Makefile,v 1.8 1997/05/25 12:05:19 darrenr Exp $

PROG=	ipnat
SRCS=	kmem.c ipnat.c
MAN=	ipnat.8 ipnat.4 ipnat.5
CFLAGS+=-I${.CURDIR}/../ipf -I${.CURDIR}/../ipfstat

.PATH: ${.CURDIR}/../ipf ${.CURDIR}/../ipfstat
.include <bsd.prog.mk>
