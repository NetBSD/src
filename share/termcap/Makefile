#	$NetBSD: Makefile,v 1.24 2002/05/30 10:45:03 itojun Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/8/93

MAN=	termcap.5

CLEANFILES= termcap.db
realall: termcap.db
FILES=termcap.db termcap.src map3270
FILESNAME_termcap.src=termcap
FILESDIR=${BINDIR}/misc

# there's no issue in endianness as DB keys are ascii strings.
termcap.db: termcap.src
	${CAP_MKDB} -f termcap ${.ALLSRC}

.include <bsd.prog.mk>
