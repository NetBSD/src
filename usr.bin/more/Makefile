#	from: @(#)Makefile	5.11 (Berkeley) 6/25/90
#	$Id: Makefile,v 1.2 1993/07/31 15:18:48 mycroft Exp $

PROG=	more
DPADD=	${LIBTERM}
LDADD=	-ltermcap
MLINKS=	more.1 page.1
LINKS=	${BINDIR}/more ${BINDIR}/page

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/more.help \
	    ${DESTDIR}/usr/share/misc/omore.help

.include <bsd.prog.mk>
