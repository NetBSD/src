#	$NetBSD: Makefile,v 1.5 1997/01/09 20:22:49 tls Exp $

CFLAGS += -I${.CURDIR}/../mkhits -I${.CURDIR}/.. -I.

NOMAN = I mean it!

SRCS = mkastosc.c dohits.c asc_ebc.c ebc_disp.c
PROG = mkastosc

.include <bsd.prog.mk>

.PATH: ${.CURDIR}/../mkhits ${.CURDIR}/../../api
