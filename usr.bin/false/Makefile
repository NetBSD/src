#	from: @(#)Makefile	5.3 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.3 1994/08/05 01:28:56 jtc Exp $

NOPROG=	false
MAN1=	false.0

install:
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		false.sh ${DESTDIR}${BINDIR}/false

.include <bsd.prog.mk>
