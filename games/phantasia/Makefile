#	$NetBSD: Makefile,v 1.20 2000/04/24 15:15:05 pk Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

.include <bsd.own.mk>		# for UNPRIVILEGED

PROG=	phantasia
SRCS=	fight.c gamesupport.c interplayer.c io.c main.c misc.c phantglobs.c

DPADD=	${LIBM} ${LIBCURSES}
LDADD=	-lm -lcurses
HIDEGAME=hidegame
SETGIDGAME=yes
USETBL=
MAN=	phantasia.6
CLEANFILES+=map setup setup.o host_phantglobs.o

realall: setup phantasia

setup: host_phantglobs.o setup.o monsters.asc ${LIBM} 
	${HOST_LINK.c} host_phantglobs.o setup.o -o ${.TARGET} -lm

host_phantglobs.o: ${.CURDIR}/phantglobs.c
	${HOST_COMPILE.c} -o host_phantglobs.o ${.CURDIR}/phantglobs.c

setup.o: ${.CURDIR}/setup.c
	${HOST_COMPILE.c} ${.CURDIR}/setup.c

afterinstall:
	DESTDIR=${DESTDIR} ./setup -m ${.CURDIR}/monsters.asc
.if !defined(UNPRIVILEGED)
	chown games:games ${DESTDIR}/var/games/phantasia/*
.endif

# Make Phantasia map.  Change the map commands reflect your installation.
# PLOTDEVICE is used for plotting the map.  Change as appropriate.

map: map.c
	${CC} -O ${.CURDIR}/map.c -lplot -o ${.TARGET}
	./map | plot > /dev/tty

.include <bsd.prog.mk>
