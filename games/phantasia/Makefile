#	$NetBSD: Makefile,v 1.9 1997/05/31 21:21:17 cjs Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	phantasia
SRCS=	main.c fight.c io.c interplayer.c gamesupport.c misc.c phantglobs.c
DPADD=	${LIBM} ${LIBCURSES} ${LIBTERM} ${LIBCOMPAT}
LDADD=	-lm -lcurses -ltermlib -lcompat
HIDEGAME=hidegame
MAN=	phantasia.6
CLEANFILES+=map setup setup.o host_phantglobs.o

all: setup phantasia

setup: host_phantglobs.o setup.o monsters.asc ${LIBM} 
	${HOST_CC} host_phantglobs.o setup.o -o ${.TARGET} -lm

host_phantglobs.o: ${.CURDIR}/phantglobs.c
	${HOST_COMPILE.c} -o host_phantglobs.o ${.CURDIR}/phantglobs.c

setup.o: ${.CURDIR}/setup.c
	${HOST_COMPILE.c} ${.CURDIR}/setup.c

afterinstall: 
	./setup -m ${.CURDIR}/monsters.asc
	chown games:bin ${DESTDIR}/var/games/phantasia/*

# Make Phantasia map.  Change the map commands reflect your installation.
# PLOTDEVICE is used for plotting the map.  Change as appropriate.

map: map.c
	${CC} -O ${.CURDIR}/map.c -lplot -o ${.TARGET}
	./map | plot > /dev/tty

# XXX this rule shouldn't be here.
phantasia.cat6: phantasia.6
	tbl ${.CURDIR}/phantasia.6 | nroff -man > ${.TARGET}

.include <bsd.prog.mk>
