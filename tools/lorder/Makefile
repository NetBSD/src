#	$NetBSD: Makefile,v 1.4 2001/11/19 04:47:43 perry Exp $

TIMESTAMP=	${TOOLDIR}/bin/nblorder

.PATH: ${.CURDIR}/../../usr.bin/lorder

.include <bsd.hostprog.mk>

install: ${TIMESTAMP}
${TIMESTAMP}: lorder.sh
	${HOST_INSTALL_FILE} -m ${BINMODE} ${.ALLSRC} ${.TARGET}
