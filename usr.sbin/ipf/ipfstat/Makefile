#	$NetBSD: Makefile,v 1.1.1.5 1997/07/05 05:12:46 darrenr Exp $

PROG=	ipfstat
SRCS=	kmem.c fils.c opt.c parse.c
MAN=	ipfstat.8
CFLAGS+=-I${.CURDIR}/../ipf

.PATH: ${.CURDIR}/../ipf
.include <bsd.prog.mk>
