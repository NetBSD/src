#	$NetBSD: Makefile,v 1.8 2007/01/11 15:10:08 ginsbach Exp $

.include <bsd.own.mk>

COMPATOBJ!=	cd ${.CURDIR}/../compat && ${PRINTOBJDIR}
.-include	"${COMPATOBJ}/defs.mk"

TIMESTAMP=	${TOOLDIR}/bin/${_TOOL_PREFIX}lorder

CLEANFILES+=	lorder

.PATH: ${.CURDIR}/../../usr.bin/lorder

.include <bsd.hostprog.mk>

BUILD_OSTYPE!=	uname -s
BUILD_OSVERSION!= uname -r | sed -e 's,\([0-9]*\.[0-9]*\).*,\1,'

install: ${TIMESTAMP}
${TIMESTAMP}: lorder
	${HOST_INSTALL_FILE} -m ${BINMODE} ${.ALLSRC} ${.TARGET}

lorder: lorder.sh
.if ${BUILD_OSTYPE} == "Darwin" && ${BUILD_OSVERSION} < 8.0
	sed -e "s,/bin/sh,${HOST_BSHELL},g" \
	    -e "/join/s/,/ /g" < ${.ALLSRC} > ${.TARGET}
.else
	sed -e "s,/bin/sh,${HOST_BSHELL},g" < ${.ALLSRC} > ${.TARGET}
.endif

realall: lorder
