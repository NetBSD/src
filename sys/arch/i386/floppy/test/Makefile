#	from: @(#)Makefile	5.1 (Berkeley) 6/8/92
#	$Id: Makefile,v 1.1 1994/04/18 08:47:41 cgd Exp $

SRCDIR=	${.CURDIR}/../../../../../bin/test

PROG=	test
SRCS=   test.c operators.c
CFLAGS+=-I.
LINKS=	${BINDIR}/test ${BINDIR}/[
NOMAN=

BINDIR=	/bin

# use this rule to if you update binary_ops, or unary_ops
make_op: 
	sh ${SRCDIR}/mkops

.PATH:	${SRCDIR}

.include <bsd.prog.mk>
