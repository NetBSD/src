#	$NetBSD: Makefile,v 1.14 1997/05/11 15:18:13 veego Exp $

PROG=	bc
CFLAGS+=-D_POSIX_SOURCE -I. -I${.CURDIR}
LFLAGS+=-I8
SRCS=	bc.y execute.c getopt.c getopt1.c global.c load.c main.c number.c \
	scan.l storage.c util.c
LDADD+=	-ll
DPADD+=	${LIBL}

CLEANFILES+= y.tab.h

.include <bsd.prog.mk>
