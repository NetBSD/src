# $NetBSD: Makefile.inc,v 1.1.2.1 1996/06/08 15:28:39 is Exp $

BINDIR=/usr/share/keymaps/amiga
NOMAN=

PROG=${_THISDIR_}-kbdmap
MAP=${_THISDIR_}.map

realinstall: ${PROG}
.if !.exists(${BINDIR}(
	mkdir -p ${BINDIR}
.endif
	./${PROG} > ${BINDIR}/${MAP}
	chmod 555 ${BINDIR}/${MAP}
	chown ${BINOWN}.${BINGRP} ${BINDIR}/${MAP}
