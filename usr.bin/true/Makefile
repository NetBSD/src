#	from: @(#)Makefile	5.3 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.5 1994/12/22 12:31:16 cgd Exp $

MAN=	true.1

install:
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/true.sh ${DESTDIR}${BINDIR}/true

.include <bsd.prog.mk>
