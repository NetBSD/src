#	$NetBSD: Makefile,v 1.3 1999/08/02 05:36:05 erh Exp $

PROG=	sup

SRCS=	supcmain.c supcvers.c supcparse.c supcname.c supcmisc.c supcmeat.c \
	read_line.c

.PATH:	${.CURDIR}/../source

DPADD=	${LIBSUP} ${LIBCRYPT}
LDADD=	${SUPLIB} -lcrypt -lutil

sup: .NOPATH

.include <bsd.prog.mk>
