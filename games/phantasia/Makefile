#	$NetBSD: Makefile,v 1.26 2002/03/05 21:31:23 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

.include <bsd.own.mk>

PROG=	phantasia
SRCS=	fight.c gamesupport.c interplayer.c io.c main.c misc.c phantglobs.c

DPADD=	${LIBM} ${LIBCURSES}
LDADD=	-lm -lcurses
HIDEGAME=hidegame
SETGIDGAME=yes
USETBL=
MAN=	phantasia.6

FILESDIR=/var/games/phantasia
FILESOWN=games
FILESGRP=games
FILESMODE=0660
ALLFILES=gold lastdead mess monsters void motd characs scoreboard
FILES=gold lastdead mess monsters void motd
# don't overwrite existing characters or scorefile
.for file in characs scoreboard
.if !exists(${DESTDIR}/${FILESDIR}/${file})
FILES+=${file}
.endif
.endfor

CLEANFILES+=map setup setup.lo host_phantglobs.lo ${ALLFILES}

realall: ${FILES}

${FILES}: setup
	./setup -m ${.CURDIR}/monsters.asc

setup: host_phantglobs.lo setup.lo monsters.asc ${LIBM} 
	${HOST_LINK.c} host_phantglobs.lo setup.lo -o ${.TARGET} -lm

host_phantglobs.lo: ${.CURDIR}/phantglobs.c
	${HOST_COMPILE.c} -o host_phantglobs.lo ${.CURDIR}/phantglobs.c

setup.lo: ${.CURDIR}/setup.c
	${HOST_COMPILE.c} -o setup.lo ${.CURDIR}/setup.c

# Make Phantasia map.  Change the map commands reflect your installation.
# PLOTDEVICE is used for plotting the map.  Change as appropriate.

map: map.c
	${CC} -O ${.CURDIR}/map.c -lplot -o ${.TARGET}
	./map | plot > /dev/tty

.include <bsd.prog.mk>
