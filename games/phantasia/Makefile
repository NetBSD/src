#	$NetBSD: Makefile,v 1.31 2003/10/21 10:01:19 lukem Exp $
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
.if !exists(${DESTDIR}${FILESDIR}/${file})
FILES+=${file}
.endif
.endfor

CLEANFILES+=map setup setup.lo host_phantglobs.lo ${ALLFILES}

realall: ${FILES}

${FILES}: setup
	${_MKTARGET_CREATE}
	./setup -m ${.CURDIR}/monsters.asc

setup: host_phantglobs.lo setup.lo monsters.asc ${LIBM} 
	${_MKTARGET_LINK}
	${HOST_LINK.c} host_phantglobs.lo setup.lo -o ${.TARGET} -lm

BUILDSYMLINKS+=	phantglobs.c host_phantglobs.c
HOST_CPPFLAGS+=	-I${.CURDIR}

# Make Phantasia map.  Change the map commands reflect your installation.
# PLOTDEVICE is used for plotting the map.  Change as appropriate.

map: map.c
	${CC} -O ${.CURDIR}/map.c -lplot -o ${.TARGET}
	./map | plot > /dev/tty

.include <bsd.prog.mk>
