#	$NetBSD: Makefile,v 1.1.2.1 1997/11/13 17:10:48 veego Exp $

PROG=	sup

SRCS=	supcmain.c supcvers.c supcparse.c supcname.c supcmisc.c supcmeat.c \
	read_line.c

.PATH:	${.CURDIR}/../source

DPADD=	${LIBSUP} ${LIBCRYPT}
LDADD=	${SUPLIB} -lcrypt

sup: .NOPATH

.include <bsd.prog.mk>
