#	$NetBSD: Makefile,v 1.4 1997/03/29 20:00:36 thorpej Exp $

PROG=	ipfstat
SRCS=	kmem.c fils.c opt.c parse.c
MAN=	ipfstat.8
CFLAGS+=-I${.CURDIR}/../../../sbin/ipf

.PATH: ${.CURDIR}/../../../sbin/ipf
.include <bsd.prog.mk>
