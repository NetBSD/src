#	$NetBSD: Makefile,v 1.7 1996/10/18 06:18:00 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

MAN=	mkdep.1

beforeinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/mkdep.gcc.sh ${DESTDIR}/usr/bin/mkdep

.include <bsd.prog.mk>
