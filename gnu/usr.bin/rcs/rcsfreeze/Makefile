#	$NetBSD: Makefile,v 1.5 1996/10/18 03:07:39 thorpej Exp $

MAN=	rcsfreeze.1

afterinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/rcsfreeze.sh ${DESTDIR}${BINDIR}/rcsfreeze

.include "../../Makefile.inc"
.include <bsd.prog.mk>
