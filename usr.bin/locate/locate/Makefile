#	from: @(#)Makefile	5.2 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.2 1993/07/31 15:20:47 mycroft Exp $

PROG=	locate

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/updatedb.csh ${DESTDIR}/usr/libexec/locate.updatedb

.include "../../Makefile.inc"
.include <bsd.prog.mk>
