#	$Id: Makefile,v 1.3 1994/12/22 09:45:36 cgd Exp $

MAN=	rcsfreeze.1

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/rcsfreeze.sh ${DESTDIR}${BINDIR}/rcsfreeze

.include "../../Makefile.inc"
.include <bsd.prog.mk>
