#	from: @(#)Makefile	5.1.1.1 (Berkeley) 5/7/91
#	$Id: Makefile,v 1.5 1993/09/30 22:59:32 jtc Exp $

# Missing: connectives, words

FILES=	README propernames web2 web2a
NOOBJ=	noobj
LINKS=	${BINDIR}/dict/web2 ${BINDIR}/dict/words

all clean cleandir depend lint tags:

beforeinstall:
	@install -d -o root -g wheel -m 755 ${DESTDIR}${BINDIR}/dict

realinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
	    ${DESTDIR}${BINDIR}/dict

.include <bsd.prog.mk>
