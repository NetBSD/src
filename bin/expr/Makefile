#	$Header: /cvsroot/src/bin/expr/Makefile,v 1.6 1993/07/20 20:16:25 jtc Exp $

PROG=		expr
SRCS=		expr.c
CFLAGS+=	-I.
LDADD=		-lgnuregex
DPADD=		/usr/lib/libgnuregex.a
CLEANFILES+=	expr.c y.tab.h

expr.c:
	${YACC} -d ${.IMPSRC}
	mv y.tab.c expr.c

.include <bsd.prog.mk>

