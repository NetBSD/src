#	$NetBSD: Makefile,v 1.11 1997/04/29 23:30:15 phil Exp $

PROG=	bc
CFLAGS+=-D_POSIX_SOURCE -I. -I${.CURDIR}
LFLAGS+=-I8
SRCS=	bc.c scan.c util.c main.c number.c storage.c load.c execute.c \
	global.c getopt.c getopt1.c

LDADD+=	-ll
DPADD+=	${LIBL}

CLEANFILES+= bc.c scan.c bc.h

beforedepend: bc.h
bc.c bc.h: bc.y
	${YACC} -d ${.ALLSRC}
	mv y.tab.c bc.c
	mv y.tab.h bc.h

.include <bsd.prog.mk>
