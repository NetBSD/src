#	$Id: Makefile,v 1.1.1.1 1995/07/03 20:56:39 cgd Exp $

LIBS=	llib-lposix.ln llib-lstdc.ln

all:	${LIBS}

install:
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${NONBINMODE} \
	    ${LIBS} ${DESTDIR}${LINTLIBDIR}

clean cleanall:
	rm -f ${LIBS}

llib-lposix.ln: llib-lposix
	lint -Cposix ${.ALLSRC}

llib-lstdc.ln: llib-lstdc
	lint -Cstdc ${.ALLSRC}

.include <bsd.prog.mk>
