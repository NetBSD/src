#	$NetBSD: Makefile,v 1.4 1996/10/18 06:16:41 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	locate

beforeinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/updatedb.csh ${DESTDIR}/usr/libexec/locate.updatedb

.include "../../Makefile.inc"
.include <bsd.prog.mk>
