#	$NetBSD: Makefile,v 1.5 1997/05/15 10:08:47 veego Exp $

LIBS=	llib-lposix.ln llib-lstdc.ln
FILES=	${LIBS}
FILESDIR= ${LINTLIBDIR}

all:	${LIBS}

CLEANFILES+= ${LIBS}

llib-lposix.ln: llib-lposix
	lint -Cposix ${.ALLSRC}

llib-lstdc.ln: llib-lstdc
	lint -Cstdc ${.ALLSRC}

.include <bsd.prog.mk>
