# $Id: Makefile,v 1.5 2003/12/19 18:39:04 veego Exp $

NOMAN=		# defined

PROG=		divremtest
COPTS+=		-Wall -Wno-format -Wno-parentheses -Wno-uninitialized
GOODRESULT=	3f181846a7fd931b9aeafeac6b32d24c

regress: ${PROG}
	[ `./${PROG} | md5` = ${GOODRESULT} ]

.include <bsd.prog.mk>
