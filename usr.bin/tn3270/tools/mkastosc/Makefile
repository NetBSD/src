#	$NetBSD: Makefile,v 1.6 1997/05/06 20:45:45 gwr Exp $

CFLAGS += -I${.CURDIR}/../mkhits -I${.CURDIR}/.. -I.

NOMAN = I mean it!

SRCS = mkastosc.c dohits.c asc_ebc.c ebc_disp.c
PROG = mkastosc

.include <bsd.prog.mk>

.PATH.c: ${.CURDIR}/../mkhits ${.CURDIR}/../../api
