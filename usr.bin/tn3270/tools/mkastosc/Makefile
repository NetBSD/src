#	$NetBSD: Makefile,v 1.9 1999/07/14 21:36:37 wrstuden Exp $

HOSTPROG=	mkastosc
SRCS=	mkastosc.c dohits.c asc_ebc.c ebc_disp.c

HOST_CPPFLAGS+=-I${.CURDIR}/../mkhits
.PATH: ${.CURDIR}/../mkhits

.include <bsd.hostprog.mk>

