#	from: @(#)Makefile	8.2 (Berkeley) 4/22/94
#	$Id: Makefile,v 1.3 1994/12/22 10:47:42 cgd Exp $

MAN=	nologin.8

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/nologin.sh ${DESTDIR}/sbin/nologin

.include <bsd.prog.mk>
