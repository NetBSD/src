#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$Id: Makefile,v 1.5 1996/10/18 06:15:20 thorpej Exp $

PROG=	id
MAN=	id.1 groups.1 whoami.1

afterinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/groups.sh ${DESTDIR}/usr/bin/groups
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/whoami.sh ${DESTDIR}/usr/bin/whoami

.include <bsd.prog.mk>
