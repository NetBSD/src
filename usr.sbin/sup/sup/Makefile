#	$NetBSD: Makefile,v 1.5 2002/11/30 03:10:58 lukem Exp $

PROG=	sup
SRCS=	supcmain.c supcvers.c supcparse.c supcname.c supcmisc.c supcmeat.c \
	read_line.c

.PATH:	${.CURDIR}/../source

DPADD=	${LIBSUP} ${LIBCRYPT}
LDADD=	${SUPLIB} -lcrypt

.include <bsd.prog.mk>
