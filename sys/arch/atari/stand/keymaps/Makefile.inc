# $NetBSD: Makefile.inc,v 1.3 1996/07/15 06:35:43 leo Exp $
MAN=
INSTDIR=/usr/share/keymaps/atari

all: ${PROG} ${MAP}

${MAP}: ${PROG}

realinstall: ${MAP}
	./${PROG} > ${DESTDIR}${INSTDIR}/${MAP}
	chmod 444 ${DESTDIR}${INSTDIR}/${MAP}
	chown ${BINOWN}.${BINGRP} ${DESTDIR}${INSTDIR}/${MAP}
