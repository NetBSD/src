#	$NetBSD: Makefile,v 1.1 2001/09/22 04:15:20 tv Exp $

TIMESTAMP=	${TOOLDIR}/bin/lorder

.PATH: ${.CURDIR}/../../usr.bin/lorder

install: ${TIMESTAMP}
${TIMESTAMP}: lorder.sh
.if !make(build)
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m ${BINMODE} $> $@
.endif

.include <bsd.hostprog.mk>
