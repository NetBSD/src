#	$NetBSD: Makefile,v 1.3 2005/06/05 18:19:53 thorpej Exp $

.include <bsd.own.mk>

.PATH.sh:	${.CURDIR}/../../usr.bin/genassym

COMPATOBJ!=	cd ${.CURDIR}/../compat && ${PRINTOBJDIR}
.-include	"${COMPATOBJ}/defs.mk"


TIMESTAMP=	${TOOLDIR}/bin/${_TOOL_PREFIX}genassym

CLEANFILES+=	genassym

.include <bsd.hostprog.mk>

install: ${TIMESTAMP}
${TIMESTAMP}: genassym
	${HOST_INSTALL_FILE} -m ${BINMODE} ${.ALLSRC} ${.TARGET}

genassym: genassym.sh
	sed -e "s,/bin/sh,${HOST_BSHELL},g" < ${.ALLSRC} > ${.TARGET}

realall: genassym
