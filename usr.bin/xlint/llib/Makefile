#	$NetBSD: Makefile,v 1.7.2.1 2000/08/02 10:24:25 mrg Exp $

.include <bsd.own.mk>

LIBS=	llib-lposix.ln llib-lstdc.ln

.if ${MKLINT} != "no"
FILES=	${LIBS}
FILESDIR= ${LINTLIBDIR}

realall:	${LIBS}
.endif

CLEANFILES+= ${LIBS}

llib-lposix.ln: llib-lposix
	${LINT} ${LINTFLAGS} -Cposix ${.ALLSRC}

llib-lstdc.ln: llib-lstdc
	${LINT} ${LINTFLAGS} -Cstdc ${.ALLSRC}

.include <bsd.prog.mk>
