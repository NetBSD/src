#	$NetBSD: Makefile,v 1.8 1996/10/18 05:51:00 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

MAN=	fastboot.8
MLINKS=	fastboot.8 fasthalt.8

beforeinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/fastboot.sh ${DESTDIR}${BINDIR}/fastboot
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/fasthalt.sh ${DESTDIR}${BINDIR}/fasthalt

.include <bsd.prog.mk>
