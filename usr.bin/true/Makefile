#	from: @(#)Makefile	5.3 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.7 1996/10/18 06:22:36 thorpej Exp $

MAN=	true.1

beforeinstall:
	${INSTALL} ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/true.sh ${DESTDIR}${BINDIR}/true

.include <bsd.prog.mk>
