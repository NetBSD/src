#	$Id: Makefile,v 1.2 1996/10/18 05:49:46 thorpej Exp $

NOMAN=
NOPROG=
NOOBJ=

afterinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		$(.CURDIR)/makewhatis.sh $(DESTDIR)$(BINDIR)/makewhatis

.include <bsd.prog.mk>
