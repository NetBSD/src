# $Id: Makefile,v 1.2 2002/01/23 20:48:08 ross Exp $

PROG=		divremtest
NOMAN=		# defined
COPTS+=		-Wall -Wno-format -Wno-parentheses
GOODRESULT=	ffc1f640297c048e4a9ee02c89bd328a

regress: ${PROG}
	[ `./${PROG} | md5` = ${GOODRESULT} ]

.include <bsd.prog.mk>
