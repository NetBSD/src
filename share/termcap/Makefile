#	$NetBSD: Makefile,v 1.23 2002/03/30 19:09:52 bjh21 Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/8/93

MAN=	termcap.5

CLEANFILES= termcap.db
realall: termcap.db
FILES=termcap.db termcap.src map3270
FILESNAME_termcap.src=termcap
FILESDIR=${BINDIR}/misc

termcap.db: termcap.src
	${CAP_MKDB} -f termcap ${.ALLSRC}

.include <bsd.prog.mk>
