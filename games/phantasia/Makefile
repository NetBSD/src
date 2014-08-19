#	$NetBSD: Makefile,v 1.35.8.2 2014/08/20 00:00:23 tls Exp $
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

CLEANFILES+=map mkdata setup.lo host_phantglobs.lo ${ALLFILES} files.stamp

realall: ${FILES}

${FILES}: files.stamp
files.stamp: mkdata monsters.asc
	${_MKMSG_CREATE} ${FILES}
	rm -f ${.TARGET}
	./mkdata -m ${.CURDIR}/monsters.asc
	touch ${.TARGET}

mkdata: host_phantglobs.lo setup.lo ${LIBM} 
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

.if defined(HAVE_GCC) && ${MACHINE_ARCH} == "vax"
COPTS.misc.c+=	-O0
.endif
