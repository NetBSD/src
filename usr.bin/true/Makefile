#	from: @(#)Makefile	5.3 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.6 1994/12/22 13:10:45 cgd Exp $

MAN=	true.1

beforeinstall:
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/true.sh ${DESTDIR}${BINDIR}/true

.include <bsd.prog.mk>
