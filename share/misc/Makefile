#	from: @(#)Makefile	5.13 (Berkeley) 5/7/91
#	$Id: Makefile,v 1.4 1993/09/30 23:31:36 jtc Exp $

FILES=	airport ascii birthtoken eqnchar inter.phone man.template \
	mdoc.template na.phone operator zipcodes
NOOBJ=	noobj

all clean cleandir depend lint tags:

beforeinstall:
	@install -d -o root -g wheel -m 755 ${DESTDIR}${BINDIR}/misc

realinstall:
	cd ${.CURDIR}; install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
	    ${DESTDIR}${BINDIR}/misc

.include <bsd.prog.mk>
