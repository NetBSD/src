# $NetBSD: Makefile.inc,v 1.2 1996/06/22 21:01:29 leo Exp $
MAN=
INSTDIR=/usr/share/keymap/atari

all: ${PROG} ${MAP}

${MAP}: ${PROG}

realinstall: ${MAP}
	./${PROG} > ${DESTDIR}${INSTDIR}/${MAP}
	chmod 444 ${DESTDIR}${INSTDIR}/${MAP}
	chown ${BINOWN}.${BINGRP} ${DESTDIR}${INSTDIR}/${MAP}
