#	from: @(#)Makefile	5.3 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.4 1994/08/20 08:41:09 pk Exp $

NOPROG=	false
MAN1=	false.0

install:
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/false.sh ${DESTDIR}${BINDIR}/false

.include <bsd.prog.mk>
