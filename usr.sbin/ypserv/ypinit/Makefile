#	$NetBSD: Makefile,v 1.4 1997/01/27 00:28:31 cjs Exp $

MAN= ypinit.8

beforeinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/ypinit.sh ${DESTDIR}/usr/sbin/ypinit
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 0644 \
	    ${.CURDIR}/Makefile.yp ${DESTDIR}/var/yp/Makefile.yp
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 0644 \
	    ${.CURDIR}/Makefile.main ${DESTDIR}/var/yp/Makefile.main

.include <bsd.prog.mk>
