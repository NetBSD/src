#	from: @(#)Makefile	5.4 (Berkeley) 2/19/91
#	$Id: Makefile,v 1.3 1994/02/15 17:09:31 pk Exp $

MAN1=	mkdep.0

.if (${MACHINE} == "hp300" || ${MACHINE} == "i386" || ${MACHINE} == "sparc")
beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/mkdep.gcc.sh ${DESTDIR}/usr/bin/mkdep
.else
beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/mkdep.sh ${DESTDIR}/usr/bin/mkdep
.endif

.include <bsd.prog.mk>
