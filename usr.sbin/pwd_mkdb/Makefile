#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$Id: Makefile,v 1.5 1994/12/22 11:38:49 cgd Exp $

PROG=	pwd_mkdb
SRCS=	pw_scan.c pwd_mkdb.c getpwent.c
MAN=	pwd_mkdb.8

.PATH:	${.CURDIR}/../../lib/libc/gen

.include <bsd.prog.mk>

getpwent.o: getpwent.c
	${CC} ${CFLAGS} ${CPPFLAGS} -UYP -c ${.IMPSRC}
