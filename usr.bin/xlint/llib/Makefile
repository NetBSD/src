#	$NetBSD: Makefile,v 1.4 1997/03/24 22:00:06 christos Exp $

LIBS=	llib-lposix.ln llib-lstdc.ln
FILES=	${LIBS}
FILESDIR= ${LINTLIBDIR}

all:	${LIBS}

clean cleanall:
	rm -f ${LIBS}

llib-lposix.ln: llib-lposix
	lint -Cposix ${.ALLSRC}

llib-lstdc.ln: llib-lstdc
	lint -Cstdc ${.ALLSRC}

.include <bsd.prog.mk>
