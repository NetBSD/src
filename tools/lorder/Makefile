#	$NetBSD: Makefile,v 1.5 2002/09/15 17:05:26 thorpej Exp $

.include <bsd.own.mk>

COMPATOBJ!=	cd ${.CURDIR}/../compat && ${PRINTOBJDIR}
.-include	"${COMPATOBJ}/defs.mk"

TIMESTAMP=	${TOOLDIR}/bin/nblorder

CLEANFILES+=	lorder

.PATH: ${.CURDIR}/../../usr.bin/lorder

.include <bsd.hostprog.mk>

install: ${TIMESTAMP}
${TIMESTAMP}: lorder
	${HOST_INSTALL_FILE} -m ${BINMODE} ${.ALLSRC} ${.TARGET}

lorder: lorder.sh
	sed -e "s,/bin/sh,${HOST_BSHELL},g" < ${.ALLSRC} > ${.TARGET}

realall: lorder
