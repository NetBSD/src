#	$NetBSD: Makefile,v 1.1.1.1 1996/08/09 10:14:59 thorpej Exp $

NOOBJ=	noobj

install:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/ypinit.sh ${DESTDIR}/usr/sbin/ypinit
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/Makefile.yp ${DESTDIR}/var/yp/Makefile.yp
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/Makefile.main ${DESTDIR}/var/yp/Makefile.main

.include <bsd.prog.mk>
