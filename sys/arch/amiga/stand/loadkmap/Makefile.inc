# $NetBSD: Makefile.inc,v 1.1.2.2 1996/06/08 15:41:58 is Exp $

BINDIR=/usr/share/keymaps/amiga
NOMAN=

realinstall: ${PROG}
.if !.exists(${BINDIR}(
	mkdir -p ${BINDIR}
.endif
	./${PROG} > ${BINDIR}/${MAP}
	chmod 555 ${BINDIR}/${MAP}
	chown ${BINOWN}.${BINGRP} ${BINDIR}/${MAP}
