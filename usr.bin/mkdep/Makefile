#	from: @(#)Makefile	5.4 (Berkeley) 2/19/91
#	$Id: Makefile,v 1.5 1994/12/22 12:30:21 cgd Exp $

MAN=	mkdep.1

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
