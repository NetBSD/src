#	$NetBSD: Makefile,v 1.6 1997/05/06 20:46:20 gwr Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	pac
CFLAGS+=-I${.CURDIR}/../common_source
MAN=	pac.8
SRCS=	pac.c common.c
.PATH.c: ${.CURDIR}/../common_source

.include "../../Makefile.inc"
.include <bsd.prog.mk>
