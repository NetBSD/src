#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$Id: Makefile,v 1.4 1994/08/28 23:32:47 mycroft Exp $

PROG=	pwd_mkdb
SRCS=	pw_scan.c pwd_mkdb.c getpwent.c
MAN8=	pwd_mkdb.0

.PATH:	${.CURDIR}/../../lib/libc/gen

.include <bsd.prog.mk>

getpwent.o: getpwent.c
	${CC} ${CFLAGS} ${CPPFLAGS} -UYP -c ${.IMPSRC}
