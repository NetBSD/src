#	from: @(#)Makefile	5.3 (Berkeley) 5/18/90
#	$Id: Makefile,v 1.6 1994/12/22 12:30:12 cgd Exp $

MAN=	lorder.1

realinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/lorder.sh ${DESTDIR}${BINDIR}/lorder

.include <bsd.prog.mk>
