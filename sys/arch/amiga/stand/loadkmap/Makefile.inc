# $NetBSD: Makefile.inc,v 1.1.2.3 1996/06/25 01:13:36 jtc Exp $

BINDIR=$(DESTDIR)/usr/share/keymaps/amiga
NOMAN=

realinstall: ${PROG}
.if !.exists(${BINDIR}(
	mkdir -p ${BINDIR}
.endif
	./${PROG} > ${BINDIR}/${MAP}
	chmod 555 ${BINDIR}/${MAP}
	chown ${BINOWN}.${BINGRP} ${BINDIR}/${MAP}
