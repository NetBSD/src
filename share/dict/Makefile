#	from: @(#)Makefile	5.1.1.1 (Berkeley) 5/7/91
#	$Id: Makefile,v 1.4 1993/08/10 16:42:48 mycroft Exp $

# Missing: connectives, words

FILES=	README propernames web2 web2a
NOOBJ=	noobj
LINKS=	${BINDIR}/dict/web2 ${BINDIR}/dict/words

all clean cleandir depend lint tags:

realinstall:
	@@if [ ! -d ${DESTDIR}${BINDIR}/dict ]; then \
                /bin/rm -f ${DESTDIR}${BINDIR}/dict ; \
                mkdir -p ${DESTDIR}${BINDIR}/dict ; \
                chown root.wheel ${DESTDIR}${BINDIR}/dict ; \
                chmod 755 ${DESTDIR}${BINDIR}/dict ; \
        else \
                true ; \
        fi
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
	    ${DESTDIR}${BINDIR}/dict

.include <bsd.prog.mk>
