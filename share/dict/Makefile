#	@(#)Makefile	5.1.1.1 (Berkeley) 5/7/91

# Missing: connectives, words

FILES=	README propernames web2 web2a
NOOBJ=	noobj

all clean cleandir depend lint tags:

install:
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
