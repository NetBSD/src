#	$NetBSD: Makefile,v 1.2 2001/09/22 05:02:13 tv Exp $

TIMESTAMP=	${TOOLDIR}/bin/lorder

.PATH: ${.CURDIR}/../../usr.bin/lorder

.include <bsd.hostprog.mk>

install: ${TIMESTAMP}
${TIMESTAMP}: lorder.sh
.if !make(build)
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m ${BINMODE} $> $@
.endif
