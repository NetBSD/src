#	$NetBSD: Makefile,v 1.7 1996/10/18 03:03:09 thorpej Exp $

MAN=		grog.1

beforeinstall:
	${INSTALL} ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		$(.CURDIR)/grog.sh $(DESTDIR)$(BINDIR)/grog

.include <../../Makefile.inc>
.include <bsd.prog.mk>
