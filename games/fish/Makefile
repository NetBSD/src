#	from: @(#)Makefile	5.5 (Berkeley) 1/18/91
#	$Id: Makefile,v 1.3 1994/12/15 08:12:59 jtc Exp $

PROG=	fish
MAN6=	fish.0
HIDEGAME=hidegame

beforeinstall: 
	install -c -o bin -g bin -m 444 ${.CURDIR}/fish.instr \
		${DESTDIR}/usr/share/games

.include <bsd.prog.mk>
