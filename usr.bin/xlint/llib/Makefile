#	$NetBSD: Makefile,v 1.3 1996/10/18 06:25:48 thorpej Exp $

LIBS=	llib-lposix.ln llib-lstdc.ln

all:	${LIBS}

install:
	${INSTALL} ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${NONBINMODE} \
	    ${LIBS} ${DESTDIR}${LINTLIBDIR}

clean cleanall:
	rm -f ${LIBS}

llib-lposix.ln: llib-lposix
	lint -Cposix ${.ALLSRC}

llib-lstdc.ln: llib-lstdc
	lint -Cstdc ${.ALLSRC}

.include <bsd.prog.mk>
