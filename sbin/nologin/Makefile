#	from: @(#)Makefile	8.2 (Berkeley) 4/22/94
#	$Id: Makefile,v 1.2 1994/09/23 01:47:32 mycroft Exp $

MAN8=	nologin.0

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/nologin.sh ${DESTDIR}/sbin/nologin

.include <bsd.prog.mk>
