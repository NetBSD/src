#	$Id: Makefile,v 1.4 1994/12/22 09:45:09 cgd Exp $

MAN=		nroff.1

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		$(.CURDIR)/nroff.sh $(DESTDIR)$(BINDIR)/nroff
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		$(.CURDIR)/psroff.sh $(DESTDIR)$(BINDIR)/psroff

.include <../../Makefile.inc>
.include <../Makefile.cfg>
.include <bsd.prog.mk>
