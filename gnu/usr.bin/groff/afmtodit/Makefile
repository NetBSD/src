#	$NetBSD: Makefile,v 1.5 1996/10/18 03:01:20 thorpej Exp $

MAN=		afmtodit.1

afterinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		$(.CURDIR)/afmtodit.pl $(DESTDIR)$(BINDIR)/afmtodit

.include <../../Makefile.inc>
.include <../Makefile.cfg>
.include <bsd.prog.mk>
