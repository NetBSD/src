#	$Id: Makefile,v 1.3 1993/03/22 08:04:00 cgd Exp $

PROG=expr
SRCS=expr.c
CFLAGS+=-I. -I${.CURDIR}
NOMAN=noman
CLEANFILES+=expr.c y.tab.h

expr.c:
	${YACC} -d ${.IMPSRC}
	mv y.tab.c expr.c

.include <bsd.prog.mk>

