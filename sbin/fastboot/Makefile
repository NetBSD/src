#	from: @(#)Makefile	8.1 (Berkeley) 6/5/93
#	$Id: Makefile,v 1.5 1994/09/23 01:38:45 mycroft Exp $

MAN8=	fastboot.0
MLINKS=	fastboot.8 fasthalt.8

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/fastboot.sh ${DESTDIR}${BINDIR}/fastboot
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/fasthalt.sh ${DESTDIR}${BINDIR}/fasthalt

.include <bsd.prog.mk>
