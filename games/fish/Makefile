#	from: @(#)Makefile	5.5 (Berkeley) 1/18/91
#	$Id: Makefile,v 1.4 1994/12/22 09:34:07 cgd Exp $

PROG=	fish
MAN=	fish.6
HIDEGAME=hidegame

beforeinstall: 
	install -c -o bin -g bin -m 444 ${.CURDIR}/fish.instr \
		${DESTDIR}/usr/share/games

.include <bsd.prog.mk>
