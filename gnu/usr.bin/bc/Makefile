#	$Id: Makefile,v 1.8 1994/03/11 20:25:06 phil Exp $

PROG=	bc
CFLAGS+=-D_POSIX_SOURCE -I. -I${.CURDIR}
LFLAGS+=-I8
SRCS=	bc.c scan.c util.c main.c number.c storage.c load.c execute.c global.c

LDADD+=	-ll -lgnumalloc
DPADD+=	${LIBL} /usr/lib/libgnumalloc.a

CLEANFILES+= bc.c scan.c y.tab.h

.include <bsd.prog.mk>
