#	from: @(#)Makefile	5.4 (Berkeley) 2/19/91
#	$Id: Makefile,v 1.4 1994/02/21 18:18:35 mycroft Exp $

MAN1=	mkdep.0

#.if (${MACHINE} != "somebody")
beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/mkdep.gcc.sh ${DESTDIR}/usr/bin/mkdep
#.else
#beforeinstall:
#	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
#	    ${.CURDIR}/mkdep.sh ${DESTDIR}/usr/bin/mkdep
#.endif

.include <bsd.prog.mk>
