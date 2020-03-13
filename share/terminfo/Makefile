#	$NetBSD: Makefile,v 1.4 2020/03/13 15:19:25 roy Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/8/93

CLEANFILES= terminfo2.cdb
realall: terminfo2.cdb
FILES=terminfo2.cdb terminfo
FILESDIR=${BINDIR}/misc

terminfo2.cdb: terminfo ${TOOL_TIC}
	${_MKTARGET_CREATE}
	${TOOL_TIC} -ax -o ${.TARGET} "${.CURDIR}/terminfo" 

.include <bsd.prog.mk>
