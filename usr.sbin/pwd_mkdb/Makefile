#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$Id: Makefile,v 1.7 1997/05/06 20:46:37 gwr Exp $

PROG=	pwd_mkdb
SRCS=	pwd_mkdb.c getpwent.c
MAN=	pwd_mkdb.8
LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.PATH.c: ${.CURDIR}/../../lib/libc/gen

.include <bsd.prog.mk>

getpwent.o: getpwent.c
	${CC} ${CFLAGS} ${CPPFLAGS} -UYP -c ${.IMPSRC}
