#	$Id: Makefile,v 1.8 1993/08/06 01:20:44 mycroft Exp $

PROG=		expr
# XXX bsd.prog.mk should be able to generate this from expr.y without SRCS
SRCS=		expr.c
CFLAGS+=	-I.
LDADD=		-lgnuregex
DPADD=		/usr/lib/libgnuregex.a
CLEANFILES+=	expr.c y.tab.h

expr.c:
	${YACC} -d ${.IMPSRC}
	mv y.tab.c expr.c

.include <bsd.prog.mk>

