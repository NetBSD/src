#	$NetBSD: Makefile,v 1.3 1996/10/18 06:32:14 thorpej Exp $

NOOBJ=	noobj

install:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/ypinit.sh ${DESTDIR}/usr/sbin/ypinit
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 0644 \
	    ${.CURDIR}/Makefile.yp ${DESTDIR}/var/yp/Makefile.yp
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 0644 \
	    ${.CURDIR}/Makefile.main ${DESTDIR}/var/yp/Makefile.main

.include <bsd.prog.mk>
