MAN1=		nroff.0

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		$(.CURDIR)/nroff.sh $(DESTDIR)$(BINDIR)/nroff
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		$(.CURDIR)/psroff.sh $(DESTDIR)$(BINDIR)/psroff

.include <../../Makefile.inc>
.include <../Makefile.cfg>
.include <bsd.prog.mk>
