#	$Id: Makefile,v 1.2 1993/08/02 17:47:54 mycroft Exp $

MAN1=	rcsfreeze.0

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/rcsfreeze.sh ${DESTDIR}${BINDIR}/rcsfreeze

.include "../../Makefile.inc"
.include <bsd.prog.mk>
