#        @(#)Makefile	5.1 (Berkeley) 6/8/92

PROG=	test
SRCS=   operators.c test.c
CFLAGS+=-I.
LINKS=	${BINDIR}/test ${BINDIR}/[
MLINKS=	test.1 '[.1'
CLEANFILES+= operators.c operators.h

operators.c operators.h: unary_op binary_op mkops
	sh ${.CURDIR}/mkops

.include <bsd.prog.mk>
