#	$Id: Makefile,v 1.4 1994/12/22 09:44:45 cgd Exp $

MAN=		afmtodit.1

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		$(.CURDIR)/afmtodit.pl $(DESTDIR)$(BINDIR)/afmtodit

.include <../../Makefile.inc>
.include <../Makefile.cfg>
.include <bsd.prog.mk>
