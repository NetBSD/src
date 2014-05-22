# $NetBSD: Makefile,v 1.1.4.2 2014/05/22 11:36:22 yamt Exp $

PROG=cgram
DPADD=${LIBCURSES} ${LIBTERMINFO}
LDADD=-lcurses -lterminfo
SRCS=cgram.c
MAN=cgram.6
HIDEGAME=hidegame

.include <bsd.prog.mk>
