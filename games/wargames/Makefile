#	from: @(#)Makefile	5.3 (Berkeley) 6/8/90
#	$Id: Makefile,v 1.3 1993/12/22 04:30:48 cgd Exp $

NOOBJ=	noobj
all wargames clean cleandir depend lint tags:

install:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/wargames.sh ${DESTDIR}/usr/games/wargames

.include <bsd.prog.mk>
