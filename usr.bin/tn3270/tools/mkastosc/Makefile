#	$Id: Makefile,v 1.4 1993/08/02 17:56:05 mycroft Exp $

CFLAGS += -I${.CURDIR}/../mkhits -I${.CURDIR}/.. -I.

NOMAN = I mean it!

SRCS = mkastosc.c dohits.c asc_ebc.c ebc_disp.c
PROG = mkastosc

.include <bsd.prog.mk>

.PATH: ${.CURDIR}/../mkhits ${.CURDIR}/../../api
