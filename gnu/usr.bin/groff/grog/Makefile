#	$Id: Makefile,v 1.3 1993/08/02 17:43:07 mycroft Exp $

MAN1=		grog.0

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		$(.CURDIR)/grog.pl $(DESTDIR)$(BINDIR)/grog

.include <../../Makefile.inc>
.include <bsd.prog.mk>
