# $NetBSD: Makefile.inc,v 1.3 1997/01/29 17:12:51 is Exp $

BINDIR=$(DESTDIR)/usr/share/keymaps/amiga
NOMAN=

realinstall: ${PROG}
.if !.exists(${BINDIR})
	mkdir -p ${BINDIR}
.endif
	./${PROG} > ${BINDIR}/${MAP}
	chmod 555 ${BINDIR}/${MAP}
	chown ${BINOWN}.${BINGRP} ${BINDIR}/${MAP}
