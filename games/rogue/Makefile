#	from: @(#)Makefile	5.7 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.2 1993/08/01 05:44:27 mycroft Exp $

PROG=	rogue
CFLAGS+=-DUNIX -DUNIX_BSD4_2 -fwritable-strings
SRCS=	curses.c hit.c init.c inventory.c level.c machdep.c main.c \
	message.c monster.c move.c object.c pack.c play.c random.c ring.c \
	room.c save.c score.c spec_hit.c throw.c trap.c use.c zap.c
DPADD=	${LIBCURSES} ${LIBTERM}
LDADD=	-lcurses -ltermlib
HIDEGAME=hidegame
MAN6=	rogue.0

.include <bsd.prog.mk>
