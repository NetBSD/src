#	$Id: Makefile,v 1.5 1996/10/18 03:05:00 thorpej Exp $

MAN=		nroff.1

afterinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		$(.CURDIR)/nroff.sh $(DESTDIR)$(BINDIR)/nroff
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		$(.CURDIR)/psroff.sh $(DESTDIR)$(BINDIR)/psroff

.include <../../Makefile.inc>
.include <../Makefile.cfg>
.include <bsd.prog.mk>
