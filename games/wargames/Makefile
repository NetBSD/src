#	$NetBSD: Makefile,v 1.5 1996/10/18 02:50:57 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

NOOBJ=	noobj
all wargames clean cleandir depend lint tags:

install:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/wargames.sh ${DESTDIR}/usr/games/wargames

.include <bsd.prog.mk>
