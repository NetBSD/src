#	$Id: Makefile,v 1.4 1994/12/22 09:44:53 cgd Exp $

MAN=		grog.1

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		$(.CURDIR)/grog.pl $(DESTDIR)$(BINDIR)/grog

.include <../../Makefile.inc>
.include <bsd.prog.mk>
