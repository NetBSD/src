#	$NetBSD: Makefile,v 1.6.2.1 2000/06/23 16:40:21 minoura Exp $

LIBS=	llib-lposix.ln llib-lstdc.ln
FILES=	${LIBS}
FILESDIR= ${LINTLIBDIR}

realall:	${LIBS}

CLEANFILES+= ${LIBS}

llib-lposix.ln: llib-lposix
	${LINT} ${LINTFLAGS} -Cposix ${.ALLSRC}

llib-lstdc.ln: llib-lstdc
	${LINT} ${LINTFLAGS} -Cstdc ${.ALLSRC}

.include <bsd.prog.mk>
