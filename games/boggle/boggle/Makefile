#	$NetBSD: Makefile,v 1.4 1996/10/18 02:44:27 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/11/93

PROG=	boggle
SRCS=	bog.c help.c mach.c prtable.c timer.c word.c
DPADD=	${LIBCURSES} ${LIBTERM}
LDADD=	-lcurses -ltermlib
HIDEGAME=hidegame
MAN=	boggle.6

beforeinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/helpfile \
	    ${DESTDIR}/usr/share/games/boggle/helpfile

.include "../../Makefile.inc"
.include <bsd.prog.mk>
