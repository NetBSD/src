#	$NetBSD: Makefile,v 1.6 1996/08/13 07:04:41 explorer Exp $

MAN=		grog.1

beforeinstall:
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		$(.CURDIR)/grog.sh $(DESTDIR)$(BINDIR)/grog

.include <../../Makefile.inc>
.include <bsd.prog.mk>
