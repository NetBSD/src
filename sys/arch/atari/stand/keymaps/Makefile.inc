# $NetBSD: Makefile.inc,v 1.2.2.2 1996/07/16 01:02:18 jtc Exp $
MAN=
INSTDIR=/usr/share/keymaps/atari

all: ${PROG} ${MAP}

${MAP}: ${PROG}

realinstall: ${MAP}
	./${PROG} > ${DESTDIR}${INSTDIR}/${MAP}
	chmod 444 ${DESTDIR}${INSTDIR}/${MAP}
	chown ${BINOWN}.${BINGRP} ${DESTDIR}${INSTDIR}/${MAP}
