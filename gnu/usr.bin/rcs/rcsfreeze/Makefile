#	$NetBSD: Makefile,v 1.4 1996/10/15 07:00:41 veego Exp $

MAN=	rcsfreeze.1

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/rcsfreeze.sh ${DESTDIR}${BINDIR}/rcsfreeze

.include "../../Makefile.inc"
.include <bsd.prog.mk>
