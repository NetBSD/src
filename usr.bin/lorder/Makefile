#	from: @(#)Makefile	5.3 (Berkeley) 5/18/90
#	$Id: Makefile,v 1.5 1993/07/31 15:20:10 mycroft Exp $

MAN1=	lorder.0

NOPROG=noprog

realinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/lorder.sh ${DESTDIR}${BINDIR}/lorder

.include <bsd.prog.mk>
