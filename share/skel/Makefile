#	from: @(#)Makefile	5.3 (Berkeley) 6/8/90
#	$Id: Makefile,v 1.3 1993/07/31 15:27:30 mycroft Exp $

FILES=	dot.cshrc dot.login dot.mailrc dot.profile dot.rhosts
NOOBJ=	noobj

all clean cleandir depend lint tags:

install:
	@if [ ! -d ${DESTDIR}${BINDIR}/skel ]; then \
                /bin/rm -f ${DESTDIR}${BINDIR}/skel ; \
                mkdir -p ${DESTDIR}${BINDIR}/skel ; \
                chown root.wheel ${DESTDIR}${BINDIR}/skel ; \
                chmod 755 ${DESTDIR}${BINDIR}/skel ; \
        else \
                true ; \
        fi
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
	    ${DESTDIR}${BINDIR}/skel

.include <bsd.prog.mk>
