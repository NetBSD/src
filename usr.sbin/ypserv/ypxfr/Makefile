#	$NetBSD: Makefile,v 1.4 1997/07/18 21:57:23 thorpej Exp $

PROG=	ypxfr
SRCS=	ypxfr.c ypdb.c yplog.c yplib_host.c ypresp_xdr.c
MAN=	ypxfr.8
.PATH:	${.CURDIR}/../common
CFLAGS+=-I${.CURDIR}/../common
WARNS=1

.include <bsd.prog.mk>
