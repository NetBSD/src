#	$NetBSD: Makefile,v 1.6 1997/05/06 20:46:07 gwr Exp $

PROG=	ipfstat
SRCS=	kmem.c fils.c opt.c parse.c
MAN=	ipfstat.8
CFLAGS+=-I${.CURDIR}/../ipf

.PATH.c: ${.CURDIR}/../ipf
.include <bsd.prog.mk>
