#	$Id: Makefile,v 1.7 1993/12/03 19:10:08 jtc Exp $

PROG=	bc
CFLAGS+=-D_POSIX_SOURCE -I. -I${.CURDIR}
SRCS=	bc.c scan.c util.c main.c number.c storage.c load.c execute.c global.c

LDADD+=	-ll -lgnumalloc
DPADD+=	${LIBL} /usr/lib/libgnumalloc.a

CLEANFILES+= bc.c scan.c y.tab.h

.include <bsd.prog.mk>
