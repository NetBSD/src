#	from: @(#)Makefile	5.3 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.7 1996/10/18 06:13:43 thorpej Exp $

MAN=	false.1

beforeinstall:
	${INSTALL} ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/false.sh ${DESTDIR}${BINDIR}/false

.include <bsd.prog.mk>
