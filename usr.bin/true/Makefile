#	from: @(#)Makefile	5.3 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.3 1994/08/05 01:28:47 jtc Exp $

NOPROG=	true
MAN1=	true.0

install:
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		true.sh ${DESTDIR}${BINDIR}/true

.include <bsd.prog.mk>
