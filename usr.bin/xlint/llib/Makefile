#	$NetBSD: Makefile,v 1.6 2000/01/23 20:59:11 mycroft Exp $

LIBS=	llib-lposix.ln llib-lstdc.ln
FILES=	${LIBS}
FILESDIR= ${LINTLIBDIR}

realall:	${LIBS}

CLEANFILES+= ${LIBS}

llib-lposix.ln: llib-lposix
	lint -Cposix ${.ALLSRC}

llib-lstdc.ln: llib-lstdc
	lint -Cstdc ${.ALLSRC}

.include <bsd.prog.mk>
