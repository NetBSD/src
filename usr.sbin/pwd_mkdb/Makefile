#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$Id: Makefile,v 1.8 1997/05/08 21:12:44 gwr Exp $

PROG=	pwd_mkdb
SRCS=	pwd_mkdb.c getpwent.c
MAN=	pwd_mkdb.8
LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.PATH:	${.CURDIR}/../../lib/libc/gen

.include <bsd.prog.mk>

getpwent.o: getpwent.c
	${CC} ${CFLAGS} ${CPPFLAGS} -UYP -c ${.IMPSRC}
