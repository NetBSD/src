#	from: @(#)Makefile	5.5 (Berkeley) 6/24/90
#	$Id: Makefile,v 1.5 1993/07/30 22:14:18 mycroft Exp $

PROG=	xinstall
MAN1=	install.0
CLEANFILES=xinstall

realinstall:
	install ${COPY} ${STRIP} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${PROG} ${DESTDIR}${BINDIR}/install

.include <bsd.prog.mk>


