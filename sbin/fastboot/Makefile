#	from: @(#)Makefile	5.6 (Berkeley) 6/23/90
#	$Id: Makefile,v 1.4 1993/08/01 05:38:00 mycroft Exp $

MAN8=	fastboot.0
MLINKS=	fastboot.8 fasthalt.8

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/fastboot.sh ${DESTDIR}${BINDIR}/fastboot
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/fasthalt.sh ${DESTDIR}${BINDIR}/fasthalt

.include <bsd.prog.mk>
