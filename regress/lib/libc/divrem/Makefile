# $Id: Makefile,v 1.4 2002/09/18 05:41:37 lukem Exp $

NOMAN=		# defined

PROG=		divremtest
COPTS+=		-Wall -Wno-format -Wno-parentheses -Wno-uninitialized
GOODRESULT=	ffc1f640297c048e4a9ee02c89bd328a

regress: ${PROG}
	[ `./${PROG} | md5` = ${GOODRESULT} ]

.include <bsd.prog.mk>
