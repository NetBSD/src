#	from: @(#)Makefile	5.14 (Berkeley) 9/15/92
#	$Id: Makefile,v 1.19 1997/03/24 21:57:11 christos Exp $

MAN=	termcap.5

CLEANFILES= termcap.db
all: termcap.db
FILES=termcap.db termcap.src map3270
FILESNAME_termcap.src=termcap
FILESDIR=${BINDIR}/misc

termcap.db: termcap.src
	cap_mkdb -f termcap ${.ALLSRC}

.include <bsd.prog.mk>
