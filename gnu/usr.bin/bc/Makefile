# $Header: /cvsroot/src/gnu/usr.bin/bc/Attic/Makefile,v 1.3 1993/05/04 07:49:22 cgd Exp $

PROG=	bc
CFLAGS+=-D_POSIX_SOURCE -I${.CURDIR}/obj -I${.CURDIR}
SRCS=	bc.c scan.c util.c main.c number.c storage.c load.c execute.c global.c
CLEANFILES+= bc.c scan.c y.tab.h

bc.c: bc.c.dist y.tab.h
	cp ${.CURDIR}/bc.c.dist bc.c
scan.c: scan.c.dist
	cp ${.CURDIR}/scan.c.dist scan.c
y.tab.h: y.tab.h.dist
	cp ${.CURDIR}/y.tab.h.dist y.tab.h

.include <bsd.prog.mk>
