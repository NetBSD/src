#	$NetBSD: Makefile,v 1.1.1.9 1998/05/17 16:29:44 veego Exp $

PROG=	ipfstat
SRCS=	kmem.c fils.c opt.c parse.c
MAN=	ipfstat.8
CPPFLAGS+=-I${.CURDIR}/../ipf

.PATH: ${.CURDIR}/../ipf
.include <bsd.prog.mk>
