# $NetBSD: Makefile,v 1.1 2013/08/04 05:42:47 dholland Exp $

PROG=cgram
DPADD=${LIBCURSES} ${LIBTERMINFO}
LDADD=-lcurses -lterminfo
SRCS=cgram.c
MAN=cgram.6
HIDEGAME=hidegame

.include <bsd.prog.mk>
