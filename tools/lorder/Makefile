#	$NetBSD: Makefile,v 1.3 2001/11/14 22:13:41 tv Exp $

TIMESTAMP=	${TOOLDIR}/bin/nblorder

.PATH: ${.CURDIR}/../../usr.bin/lorder

.include <bsd.hostprog.mk>

install: ${TIMESTAMP}
${TIMESTAMP}: lorder.sh
	${INSTALL_FILE:N-U} -m ${BINMODE} ${.ALLSRC} ${.TARGET}
