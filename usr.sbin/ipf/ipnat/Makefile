#	$NetBSD: Makefile,v 1.13 1998/01/24 19:15:32 veego Exp $

PROG=	ipnat
SRCS=	kmem.c ipnat.c
MAN=	ipnat.8 ipnat.4 ipnat.5
MLINKS=	ipnat.5 ipnat.conf.5
CPPFLAGS+=-I${.CURDIR}/../ipf -I${.CURDIR}/../ipfstat

.PATH: ${.CURDIR}/../ipf ${.CURDIR}/../ipfstat
.include <bsd.prog.mk>
