#	$NetBSD: Makefile,v 1.9 2003/10/19 04:30:30 lukem Exp $

.include <bsd.own.mk>

LIBS=	llib-lposix.ln llib-lstdc.ln

.if ${MKLINT} != "no"
FILES=	${LIBS}
FILESDIR= ${LINTLIBDIR}

realall:	${LIBS}
.endif

CLEANFILES+= ${LIBS}

llib-lposix.ln: llib-lposix
	${_MKMSGCREATE}
	${_MKCMD}\
	${LINT} ${LINTFLAGS} -Cposix ${.ALLSRC}

llib-lstdc.ln: llib-lstdc
	${_MKMSGCREATE}
	${_MKCMD}\
	${LINT} ${LINTFLAGS} -Cstdc ${.ALLSRC}

.include <bsd.prog.mk>
