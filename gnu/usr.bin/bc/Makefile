#	$NetBSD: Makefile,v 1.17 1997/10/25 10:10:23 lukem Exp $

PROG=	bc
CPPFLAGS+=-D_POSIX_SOURCE -I. -I${.CURDIR}
LFLAGS+=-I8
SRCS=	bc.y execute.c getopt.c getopt1.c global.c load.c main.c number.c \
	scan.l storage.c util.c
LDADD+=	-ll -ledit -ltermcap
DPADD+=	${LIBL}

CLEANFILES+= bc.h

bc.c bc.h: bc.y
	${YACC} -d ${.ALLSRC}
	mv y.tab.c bc.c
	mv y.tab.h bc.h

.include <bsd.prog.mk>
