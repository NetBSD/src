#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$Id: Makefile,v 1.4 1994/12/22 12:29:56 cgd Exp $

PROG=	id
MAN=	id.1 groups.1 whoami.1

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/groups.sh ${DESTDIR}/usr/bin/groups
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/whoami.sh ${DESTDIR}/usr/bin/whoami

.include <bsd.prog.mk>
