#	$NetBSD: Makefile,v 1.9.10.1 2002/12/01 12:19:19 he Exp $

HOSTPROG=	mkastosc
HOST_CPPFLAGS+=	-DHOST_TOOL
SRCS=	mkastosc.c dohits.c asc_ebc.c ebc_disp.c

HOST_CPPFLAGS+=-I${.CURDIR}/../mkhits
.PATH: ${.CURDIR}/../mkhits

.include <bsd.hostprog.mk>

