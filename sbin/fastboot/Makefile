#	@(#)Makefile	5.6 (Berkeley) 6/23/90
#
#	$Header: /cvsroot/src/sbin/fastboot/Makefile,v 1.3 1993/03/23 00:27:35 cgd Exp $

MAN8=	fastboot.0
MLINKS=	fastboot.8 fasthalt.8

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/fastboot.sh ${DESTDIR}${BINDIR}/fastboot
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/fasthalt.sh ${DESTDIR}${BINDIR}/fasthalt

.include <bsd.prog.mk>
