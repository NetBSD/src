# $NetBSD: Makefile,v 1.4 2001/12/12 01:24:16 tv Exp $

PROG=		extest
NOMAN=		# defined
SRCS=		extest.c subr_extent.c
CPPFLAGS+=	-D_EXTENT_TESTING -D__POOL_EXPOSE
CLEANFILES+=	extest.c extest.out
WARNS?=		1

.PATH:		${.CURDIR}/../../../../sys/kern

regress: ${PROG}
	./${PROG} >extest.out
	tail +4 ${.CURDIR}/extest.exp | diff - extest.out

extest.c: extest.awk tests
	awk -f ${.CURDIR}/extest.awk <${.CURDIR}/tests >extest.c

.include <bsd.prog.mk>
