#	$NetBSD: Makefile,v 1.6.22.1 2018/05/05 19:25:57 martin Exp $

.include <bsd.init.mk>

.PATH.sh:	${.CURDIR}/../../usr.bin/genassym

.-include	"${TOOLDIR}/share/compat/defs.mk"

TIMESTAMP=	${TOOLDIR}/bin/${_TOOL_PREFIX}genassym

CLEANFILES+=	genassym

.include <bsd.hostprog.mk>

install: ${TIMESTAMP}
${TIMESTAMP}: genassym
	${HOST_INSTALL_FILE} -m ${BINMODE} ${.ALLSRC} ${.TARGET}

genassym: genassym.sh
	${TOOL_SED} -e "s,/bin/sh,${HOST_BSHELL},g" \
	    -e "s,{AWK:=.*},{AWK:="${TOOL_AWK:Q}"}," \
		< ${.ALLSRC} > ${.TARGET}

realall: genassym
