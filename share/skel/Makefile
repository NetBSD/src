#	from: @(#)Makefile	5.3 (Berkeley) 6/8/90
#	$Id: Makefile,v 1.6 1994/02/10 01:55:04 cgd Exp $

FILES=	dot.cshrc dot.login dot.mailrc dot.profile dot.rhosts
NOOBJ=	noobj

all clean cleandir depend lint tags:

realinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
	    ${DESTDIR}${BINDIR}/skel

.include <bsd.prog.mk>
