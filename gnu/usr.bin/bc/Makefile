#	$NetBSD: Makefile,v 1.15.2.2 1997/11/09 22:31:44 lukem Exp $

PROG=	bc
CPPFLAGS+=-D_POSIX_SOURCE -I. -I${.CURDIR}
LFLAGS+=-I8
SRCS=	bc.y execute.c getopt.c getopt1.c global.c load.c main.c number.c \
	scan.l storage.c util.c
LDADD+=	-ll
DPADD+=	${LIBL}

CLEANFILES+= bc.h

bc.c bc.h: bc.y
	${YACC} -d ${.ALLSRC}
	mv y.tab.c bc.c
	mv y.tab.h bc.h

.include <bsd.prog.mk>
