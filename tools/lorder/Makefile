#	$NetBSD: Makefile,v 1.6 2002/12/08 20:20:01 thorpej Exp $

.include <bsd.own.mk>

COMPATOBJ!=	cd ${.CURDIR}/../compat && ${PRINTOBJDIR}
.-include	"${COMPATOBJ}/defs.mk"

TIMESTAMP=	${TOOLDIR}/bin/${_TOOL_PREFIX}lorder

CLEANFILES+=	lorder

.PATH: ${.CURDIR}/../../usr.bin/lorder

.include <bsd.hostprog.mk>

install: ${TIMESTAMP}
${TIMESTAMP}: lorder
	${HOST_INSTALL_FILE} -m ${BINMODE} ${.ALLSRC} ${.TARGET}

lorder: lorder.sh
	sed -e "s,/bin/sh,${HOST_BSHELL},g" < ${.ALLSRC} > ${.TARGET}

realall: lorder
