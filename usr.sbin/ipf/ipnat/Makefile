#	$NetBSD: Makefile,v 1.12 1998/01/23 22:32:08 veego Exp $

PROG=	ipnat
SRCS=	kmem.c ipnat.c
MAN=	ipnat.8 ipnat.4 ipnat.5
MLINKS=	ipnat.5 ipnat.conf.5
CPPFLAGS+=-I${.CURDIR}/../ipf -I${.CURDIR}/../ipfstat

BINDIR=		/sbin
LDSTATIC=	-static

.PATH: ${.CURDIR}/../ipf ${.CURDIR}/../ipfstat
.include <bsd.prog.mk>
