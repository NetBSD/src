# $Id: Makefile,v 1.1 2002/01/22 01:19:31 ross Exp $

PROG=		divremtest
NOMAN=		# defined
COPTS+=		-Wall -Wno-format -Wno-parentheses
GOODRESULT=	a8c75451201482303e1508e3b9494879

regress: ${PROG}
	[ `./${PROG} | md5` = ${GOODRESULT} ]

.include <bsd.prog.mk>
