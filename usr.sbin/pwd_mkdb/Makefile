#	@(#)Makefile	5.1 (Berkeley) 3/8/91

PROG=	pwd_mkdb
SRCS=	pw_scan.c pwd_mkdb.c getpwent.c
MAN8=	pwd_mkdb.0

.PATH:	${.CURDIR}/../../lib/libc/gen

.include <bsd.prog.mk>

getpwent.o: getpwent.c
	${CC} ${CFLAGS} -UYP -c ${.IMPSRC}
