# $NetBSD: Makefile.inc,v 1.1.1.1 1996/06/22 20:14:10 leo Exp $
MAN=
INSTDIR=/usr/share/keymaps/atari

all: ${PROG} ${MAP}

${MAP}: ${PROG}

realinstall: ${MAP}
	./${PROG} > ${DESTDIR}${INSTDIR}/${MAP}
	chmod 444 ${DESTDIR}${INSTDIR}/${MAP}
	chown ${BINOWN}.${BINGRP} ${DESTDIR}${INSTDIR}/${MAP}
