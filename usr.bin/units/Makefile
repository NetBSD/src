#	$NetBSD: Makefile,v 1.3 1996/10/18 06:23:04 thorpej Exp $

PROG=	units

beforeinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 \
	    ${.CURDIR}/units.lib ${DESTDIR}/usr/share/misc

.include <bsd.prog.mk>
