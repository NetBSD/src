#	$NetBSD: Makefile,v 1.3 1997/05/08 21:12:58 gwr Exp $

PROG=	ypxfr
SRCS=	ypxfr.c ypdb.c yplog.c yplib_host.c ypresp_xdr.c
MAN=	ypxfr.8
.PATH:	${.CURDIR}/../common
CFLAGS+=-I${.CURDIR}/../common

.include <bsd.prog.mk>
