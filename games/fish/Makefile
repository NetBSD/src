#	from: @(#)Makefile	5.5 (Berkeley) 1/18/91
#	$Id: Makefile,v 1.2 1993/08/01 05:45:31 mycroft Exp $

PROG=	fish
MAN6=	fish.0
HIDEGAME=hidegame

beforeinstall: 
	install -c -o bin -g bin -m 444 ${.CURDIR}/fish.instr /usr/share/games

.include <bsd.prog.mk>
