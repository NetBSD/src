#	$NetBSD: Makefile,v 1.6 1997/01/09 20:20:01 tls Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	id
MAN=	id.1 groups.1 whoami.1

afterinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/groups.sh ${DESTDIR}/usr/bin/groups
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/whoami.sh ${DESTDIR}/usr/bin/whoami

.include <bsd.prog.mk>
