#	$NetBSD: Makefile,v 1.10 2003/10/21 10:01:22 lukem Exp $

.include <bsd.own.mk>

LIBS=	llib-lposix.ln llib-lstdc.ln

.if ${MKLINT} != "no"
FILES=	${LIBS}
FILESDIR= ${LINTLIBDIR}

realall:	${LIBS}
.endif

CLEANFILES+= ${LIBS}

llib-lposix.ln: llib-lposix
	${_MKTARGET_CREATE}
	${LINT} ${LINTFLAGS} -Cposix ${.ALLSRC}

llib-lstdc.ln: llib-lstdc
	${_MKTARGET_CREATE}
	${LINT} ${LINTFLAGS} -Cstdc ${.ALLSRC}

.include <bsd.prog.mk>
