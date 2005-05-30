#	$NetBSD: Makefile,v 1.1 2005/05/30 15:51:36 thorpej Exp $

.include <bsd.own.mk>

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
