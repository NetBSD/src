#	from: @(#)Makefile	5.1 (Berkeley) 3/8/91
#	$Id: Makefile,v 1.3 1993/07/30 21:41:16 mycroft Exp $

PROG=	pwd_mkdb
SRCS=	pw_scan.c pwd_mkdb.c getpwent.c
MAN8=	pwd_mkdb.0

.PATH:	${.CURDIR}/../../lib/libc/gen

.include <bsd.prog.mk>

getpwent.o: getpwent.c
	${CC} ${CFLAGS} -UYP -c ${.IMPSRC}
