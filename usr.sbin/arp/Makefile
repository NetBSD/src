#	from: @(#)Makefile	8.2 (Berkeley) 4/18/94
#	$Id: Makefile,v 1.5 1994/05/17 08:55:04 pk Exp $

PROG=	arp
MAN8=	arp.0
CLEANFILES=arp4.0

.if !defined(NOMAN)
all: ${PROG} arp4.0 ${MAN8}

beforeinstall:
	install -c -o ${MANOWN} -g ${MANGRP} -m ${MANMODE} arp4.0 \
	    ${DESTDIR}${MANDIR}4/arp.0
.endif

.include <bsd.prog.mk>
