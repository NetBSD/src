MAN1=		grog.0

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		$(.CURDIR)/grog.pl $(DESTDIR)$(BINDIR)/grog

.include <../../Makefile.inc>
.include <bsd.prog.mk>
