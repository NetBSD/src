# $FreeBSD: src/usr.sbin/ndiscvt/Makefile,v 1.5 2004/02/23 20:21:21 johan Exp $

.PATH:	${.CURDIR}/../../sys/compat/ndis

.if ${MACHINE} == "i386"
PROG=	ndiscvt
.endif
SRCS=	ndiscvt.c
SRCS+=	subr_pe.c
SRCS+=	inf.c inf-token.l inf-parse.y

DPSRCS+= y.tab.h

MAN=	ndiscvt.8

WARNS?=	4

DPADD=  ${LIBL}
LDADD=  -ll


CFLAGS+=-I. -I${.CURDIR} -I${.CURDIR}/../../sys

CLEANFILES= y.output
CLEANFILES+= ndiscvt.cat8
CLEANFILES+= y.tab.c y.tab.h


y.tab.h: inf-parse.y
	${YACC} -v -d ${.CURDIR}/inf-parse.y

.include <bsd.prog.mk>
