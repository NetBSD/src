#	$NetBSD: Makefile,v 1.3 1997/03/29 19:49:02 thorpej Exp $

PROG=	ipfstat
SRCS=	kmem.c fils.c opt.c parse.c
MAN=	ipfstat.8
CFLAGS+=-I${.CURDIR}/../../../sbin/ipf -I${.CURDIR}/../../../sys

.PATH: ${.CURDIR}/../../../sbin/ipf
.include <bsd.prog.mk>
