#	$Id: Makefile,v 1.1.1.1 1994/12/01 04:24:14 phil Exp $

PROG=	bc
CFLAGS+=-D_POSIX_SOURCE -DDOT_IS_LAST -I. -I${.CURDIR}
LFLAGS+=-I8
SRCS=	bc.c scan.c util.c main.c number.c storage.c load.c execute.c global.c

LDADD+=	-ll
DPADD+=	${LIBL}

CLEANFILES+= bc.c scan.c y.tab.h

.include <bsd.prog.mk>
