#	$Id: Makefile,v 1.1 1993/12/21 03:28:00 cgd Exp $

NOMAN=
NOPROG=
NOOBJ=

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		$(.CURDIR)/makewhatis.sh $(DESTDIR)$(BINDIR)/makewhatis

.include <bsd.prog.mk>
