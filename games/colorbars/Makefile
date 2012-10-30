# $NetBSD: Makefile,v 1.3.4.2 2012/10/30 18:58:19 yamt Exp $

WARNS?=	4
PROG=  colorbars
MAN=   colorbars.6
DPADD=	${LIBCURSES} ${LIBTERMINFO}
LDADD=	-lcurses -lterminfo

.include <bsd.prog.mk>
