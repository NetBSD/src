#	from: @(#)Makefile	5.3 (Berkeley) 6/8/90
#	$Id: Makefile,v 1.7 1996/10/18 05:59:07 thorpej Exp $

FILES=	dot.cshrc dot.login dot.mailrc dot.profile dot.rhosts
NOOBJ=	noobj

all clean cleandir depend lint tags:

realinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
	    ${DESTDIR}${BINDIR}/skel

.include <bsd.prog.mk>
