#	$Id: Makefile,v 1.5 1995/03/22 17:30:04 glass Exp $

MAN=		grog.1

beforeinstall:
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		$(.CURDIR)/grog.sh $(DESTDIR)$(BINDIR)/grog

.include <../../Makefile.inc>
.include <bsd.prog.mk>
