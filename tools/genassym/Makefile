#	$NetBSD: Makefile,v 1.4 2008/10/19 22:05:23 apb Exp $

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
	sed -e "s,/bin/sh,${HOST_BSHELL},g" \
	    -e "s,{AWK:=.*},{AWK:="${TOOL_AWK:Q}"}," \
		< ${.ALLSRC} > ${.TARGET}

realall: genassym
