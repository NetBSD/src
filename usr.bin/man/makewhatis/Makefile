#	$Id: Makefile,v 1.2 1993/08/02 17:55:02 mycroft Exp $

BINDIR=		/usr/libexec
NOMAN=		noman

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		$(.CURDIR)/makewhatis.sh $(DESTDIR)$(BINDIR)/makewhatis

.include <bsd.prog.mk>
