#	$NetBSD: Makefile,v 1.1.1.1 1996/08/09 10:15:07 thorpej Exp $

PROG=	ypxfr
SRCS=	ypxfr.c ypdb.c yplog.c yplib_host.c ypresp_xdr.c
MAN=	ypxfr.8
.PATH:	${.CURDIR}/../common
CFLAGS+=-I${.CURDIR}/../common

.include <bsd.prog.mk>
