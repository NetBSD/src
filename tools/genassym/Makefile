#	$NetBSD: Makefile,v 1.2 2005/05/31 03:53:59 christos Exp $

.include <bsd.own.mk>

.PATH.sh:	${.CURDIR}/../../usr.sbin/genassym

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
