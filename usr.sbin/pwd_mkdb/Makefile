#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$Id: Makefile,v 1.10 1997/07/25 06:54:00 mrg Exp $

PROG=	pwd_mkdb
SRCS=	pwd_mkdb.c getpwent.c
MAN=	pwd_mkdb.8
LDADD+=	-lutil
DPADD+=	${LIBUTIL}
CFLAGS+=	-I${.CURDIR}/../../lib/libc/include

.PATH:	${.CURDIR}/../../lib/libc/gen

.include <bsd.prog.mk>

getpwent.o: getpwent.c
	${CC} ${CFLAGS} ${CPPFLAGS} -UYP -c ${.IMPSRC}
