# $NetBSD: Makefile.inc,v 1.4 1997/07/15 08:42:21 leo Exp $
MAN=
INSTDIR=/usr/share/keymaps/atari
BINDIR=${.CURDIR}

all: ${PROG} ${MAP}

${MAP}: ${PROG}

proginstall:

realinstall: ${MAP}
	./${PROG} > ${DESTDIR}${INSTDIR}/${MAP}
	chmod 444 ${DESTDIR}${INSTDIR}/${MAP}
	chown ${BINOWN}.${BINGRP} ${DESTDIR}${INSTDIR}/${MAP}
