#	$NetBSD: Makefile,v 1.2 1997/05/06 20:46:48 gwr Exp $

PROG=	ypxfr
SRCS=	ypxfr.c ypdb.c yplog.c yplib_host.c ypresp_xdr.c
MAN=	ypxfr.8
.PATH.c: ${.CURDIR}/../common
CFLAGS+=-I${.CURDIR}/../common

.include <bsd.prog.mk>
