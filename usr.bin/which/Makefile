#	from: @(#)Makefile	5.5 (Berkeley) 7/1/90
#	$Id: Makefile,v 1.2 1993/07/30 22:29:09 mycroft Exp $

MAN1=	which.0

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/which.csh ${DESTDIR}${BINDIR}/which

.include <bsd.prog.mk>
