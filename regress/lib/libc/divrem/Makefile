# $Id: Makefile,v 1.3 2002/01/29 17:21:46 thorpej Exp $

PROG=		divremtest
NOMAN=		# defined
COPTS+=		-Wall -Wno-format -Wno-parentheses -Wno-uninitialized
GOODRESULT=	ffc1f640297c048e4a9ee02c89bd328a

regress: ${PROG}
	[ `./${PROG} | md5` = ${GOODRESULT} ]

.include <bsd.prog.mk>
