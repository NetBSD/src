#	$NetBSD: Makefile,v 1.21 1997/10/17 10:47:42 mrg Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/8/93

MAN=	termcap.5

CLEANFILES= termcap.db
all: termcap.db
FILES=termcap.db termcap.src map3270
FILESNAME_termcap.src=termcap
FILESDIR=${BINDIR}/misc

termcap.db: termcap.src
	cap_mkdb -f termcap ${.ALLSRC}

.include <bsd.prog.mk>
