#	$Id: Makefile,v 1.7 1993/08/02 17:18:45 mycroft Exp $

PROG=		expr
CFLAGS+=	-I.
LDADD=		-lgnuregex
DPADD=		/usr/lib/libgnuregex.a
CLEANFILES+=	expr.c y.tab.h

expr.c:
	${YACC} -d ${.IMPSRC}
	mv y.tab.c expr.c

.include <bsd.prog.mk>

