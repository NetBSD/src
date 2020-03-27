#	$NetBSD: Makefile,v 1.5 2020/03/27 15:11:57 christos Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/8/93

CLEANFILES= terminfo.cdb
realall: terminfo.cdb
FILES=terminfo.cdb terminfo
FILESDIR=${BINDIR}/misc

terminfo.cdb: terminfo ${TOOL_TIC}
	${_MKTARGET_CREATE}
	${TOOL_TIC} -ax -o ${.TARGET} "${.CURDIR}/terminfo" 

.include <bsd.prog.mk>
