#	$NetBSD: Makefile,v 1.11 1997/10/25 06:58:13 lukem Exp $

PROG=	ipnat
SRCS=	kmem.c ipnat.c
MAN=	ipnat.8 ipnat.4 ipnat.5
MLINKS=	ipnat.5 ipnat.conf.5
CPPFLAGS+=-I${.CURDIR}/../ipf -I${.CURDIR}/../ipfstat

.PATH: ${.CURDIR}/../ipf ${.CURDIR}/../ipfstat
.include <bsd.prog.mk>
