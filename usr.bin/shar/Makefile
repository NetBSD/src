#	from: @(#)Makefile	5.1 (Berkeley) 5/23/90
#	$Id: Makefile,v 1.4 1993/07/30 23:51:08 mycroft Exp $

MAN1=	shar.0

NOPROG=noprog

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/shar.sh ${DESTDIR}/usr/bin/shar

.include <bsd.prog.mk>
