# $NetBSD: Makefile.inc,v 1.5 1997/05/24 20:09:49 veego Exp $

BINDIR=/usr/share/keymaps/amiga
NOMAN=

realinstall: ${PROG}
.if !exists($(DESTDIR)${BINDIR})
	mkdir -p $(DESTDIR)${BINDIR}
.endif
	./${PROG} > $(DESTDIR)${BINDIR}/${MAP}
	chmod 555 $(DESTDIR)${BINDIR}/${MAP}
	chown ${BINOWN}.${BINGRP} $(DESTDIR)${BINDIR}/${MAP}
