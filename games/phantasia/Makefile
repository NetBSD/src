#	$NetBSD: Makefile,v 1.34 2010/02/06 23:45:25 he Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

.include <bsd.own.mk>

PROG=	phantasia
SRCS=	fight.c gamesupport.c interplayer.c io.c main.c misc.c phantglobs.c

DPADD=	${LIBM} ${LIBCURSES} ${LIBTERMINFO}
LDADD=	-lm -lcurses -lterminfo
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

CLEANFILES+=map setup setup.lo host_phantglobs.lo ${ALLFILES} files.stamp

realall: ${FILES}

${FILES}: files.stamp
files.stamp: setup monsters.asc
	${_MKMSG_CREATE} ${FILES}
	rm -f ${.TARGET}
	./setup -m ${.CURDIR}/monsters.asc
	touch ${.TARGET}

setup: host_phantglobs.lo setup.lo ${LIBM} 
	${_MKTARGET_LINK}
	${HOST_LINK.c} -o ${.TARGET} ${.ALLSRC:M*.lo} -lm

BUILDSYMLINKS+=	phantglobs.c host_phantglobs.c
HOST_CPPFLAGS+=	-I${.CURDIR}

# Make Phantasia map.  Change the map commands reflect your installation.
# PLOTDEVICE is used for plotting the map.  Change as appropriate.

map: map.c
	${CC} -O ${.CURDIR}/map.c -lplot -o ${.TARGET}
	./map | plot > /dev/tty

.include <bsd.prog.mk>
