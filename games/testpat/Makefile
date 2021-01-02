# $NetBSD: Makefile,v 1.1 2021/01/02 03:21:39 nat Exp $

PROG=  testpat
MAN=   testpat.6
DPADD=	${LIBMATH} ${LIBCURSES} ${LIBTERMINFO}
LDADD=	-lcurses -lm -lterminfo

.include <bsd.prog.mk>
