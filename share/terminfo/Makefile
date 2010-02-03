#	$NetBSD: Makefile,v 1.1 2010/02/03 15:16:32 roy Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/8/93

CLEANFILES= terminfo.db
realall: terminfo.db
FILES=terminfo.db terminfo
FILESDIR=${BINDIR}/misc

terminfo.db: terminfo
	${_MKTARGET_CREATE}
	${TOOL_TIC} -x -o "${.OBJDIR}/terminfo" "${.CURDIR}/terminfo" 

.include <bsd.prog.mk>
