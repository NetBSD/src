#	$NetBSD: Makefile,v 1.2 1996/08/09 15:11:47 thorpej Exp $

NOOBJ=	noobj

install:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/ypinit.sh ${DESTDIR}/usr/sbin/ypinit
	install -c -o ${BINOWN} -g ${BINGRP} -m 0644 \
	    ${.CURDIR}/Makefile.yp ${DESTDIR}/var/yp/Makefile.yp
	install -c -o ${BINOWN} -g ${BINGRP} -m 0644 \
	    ${.CURDIR}/Makefile.main ${DESTDIR}/var/yp/Makefile.main

.include <bsd.prog.mk>
