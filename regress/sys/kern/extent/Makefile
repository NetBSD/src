# $NetBSD: Makefile,v 1.3 2001/10/31 15:57:56 tv Exp $

PROG= extest
NOMAN=

.PATH: ${.CURDIR}/../../../../sys/kern
SRCS= extest.c subr_extent.c

CPPFLAGS+= -D_EXTENT_TESTING -D__POOL_EXPOSE

WARNS?=	1

regress: ${PROG}
	./${PROG} >extest.out
	tail +4 ${.CURDIR}/extest.exp | diff - extest.out

extest.c: extest.awk tests
	awk -f ${.CURDIR}/extest.awk <${.CURDIR}/tests >extest.c

CLEANFILES+= extest.c extest.out

.include <bsd.prog.mk>
