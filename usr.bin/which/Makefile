#	from: @(#)Makefile	5.5 (Berkeley) 7/1/90
#	$Id: Makefile,v 1.4 1996/10/18 06:24:43 thorpej Exp $

MAN=	which.1

beforeinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/which.csh ${DESTDIR}${BINDIR}/which

.include <bsd.prog.mk>
