#	$Id: Makefile,v 1.3 1993/08/02 17:44:32 mycroft Exp $

MAN1=		nroff.0

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		$(.CURDIR)/nroff.sh $(DESTDIR)$(BINDIR)/nroff
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		$(.CURDIR)/psroff.sh $(DESTDIR)$(BINDIR)/psroff

.include <../../Makefile.inc>
.include <../Makefile.cfg>
.include <bsd.prog.mk>
