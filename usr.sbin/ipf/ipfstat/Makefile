#	$NetBSD: Makefile,v 1.1.1.3 1997/05/25 11:45:58 darrenr Exp $

PROG=	ipfstat
SRCS=	kmem.c fils.c opt.c parse.c
MAN=	ipfstat.8
CFLAGS+=-I${.CURDIR}/../ipf

.PATH: ${.CURDIR}/../ipf
.include <bsd.prog.mk>
