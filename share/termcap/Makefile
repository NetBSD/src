#	$NetBSD: Makefile,v 1.22 2000/01/23 22:04:17 mycroft Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/8/93

MAN=	termcap.5

CLEANFILES= termcap.db
realall: termcap.db
FILES=termcap.db termcap.src map3270
FILESNAME_termcap.src=termcap
FILESDIR=${BINDIR}/misc

termcap.db: termcap.src
	cap_mkdb -f termcap ${.ALLSRC}

.include <bsd.prog.mk>
