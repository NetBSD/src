#	@(#)Makefile	5.13 (Berkeley) 5/7/91

FILES=	airport ascii birthtoken eqnchar inter.phone man.template \
	mdoc.template na.phone operator zipcodes
NOOBJ=	noobj

all clean cleandir depend lint tags:

install:
	@if [ ! -d ${DESTDIR}${BINDIR}/misc ]; then \
                /bin/rm -f ${DESTDIR}${BINDIR}/misc ; \
                mkdir -p ${DESTDIR}${BINDIR}/misc ; \
                chown root.wheel ${DESTDIR}${BINDIR}/misc ; \
                chmod 755 ${DESTDIR}${BINDIR}/misc ; \
        else \
                true ; \
        fi
	cd ${.CURDIR}; install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
	    ${DESTDIR}${BINDIR}/misc

.include <bsd.prog.mk>
