#	$NetBSD: Makefile,v 1.21 2000/06/20 16:07:04 matt Exp $
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
CLEANFILES+=map setup setup.lo host_phantglobs.lo

realall: setup phantasia

setup: host_phantglobs.lo setup.lo monsters.asc ${LIBM} 
	${HOST_LINK.c} host_phantglobs.lo setup.lo -o ${.TARGET} -lm

host_phantglobs.lo: ${.CURDIR}/phantglobs.c
	${HOST_COMPILE.c} -o host_phantglobs.lo ${.CURDIR}/phantglobs.c

setup.lo: ${.CURDIR}/setup.c
	${HOST_COMPILE.c} -o setup.lo ${.CURDIR}/setup.c

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
