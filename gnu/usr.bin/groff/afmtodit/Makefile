#	$Id: Makefile,v 1.3 1993/08/02 17:41:49 mycroft Exp $

MAN1=		afmtodit.0

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		$(.CURDIR)/afmtodit.pl $(DESTDIR)$(BINDIR)/afmtodit

.include <../../Makefile.inc>
.include <../Makefile.cfg>
.include <bsd.prog.mk>
