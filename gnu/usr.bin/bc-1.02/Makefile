# $Header: /cvsroot/src/gnu/usr.bin/bc-1.02/Attic/Makefile,v 1.2 1993/04/10 16:09:10 cgd Exp $

PROG=	bc
CFLAGS+=-D_POSIX_SOURCE -I${.CURDIR}/obj
SRCS=	bc.c scan.c util.c main.c number.c storage.c load.c execute.c global.c
CLEANFILES+= bc.c scan.c y.tab.h

.include <bsd.prog.mk>
