#	from: @(#)Makefile	5.5 (Berkeley) 7/1/90
#	$Id: Makefile,v 1.3 1994/12/22 12:31:36 cgd Exp $

MAN=	which.1

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/which.csh ${DESTDIR}${BINDIR}/which

.include <bsd.prog.mk>
