#	$Id: Makefile,v 1.9 1993/11/11 01:30:19 jtc Exp $

PROG=		expr
# XXX bsd.prog.mk should be able to generate this from expr.y without SRCS
SRCS=		expr.c
CFLAGS+=	-I.
CLEANFILES+=	expr.c y.tab.h

expr.c:
	${YACC} -d ${.IMPSRC}
	mv y.tab.c expr.c

.include <bsd.prog.mk>

