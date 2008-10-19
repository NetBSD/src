# $NetBSD: Makefile,v 1.6 2008/10/19 22:05:20 apb Exp $

NOMAN=		# defined

.include <bsd.own.mk>

PROG=		extest
SRCS=		extest.c subr_extent.c
CPPFLAGS+=	-D_EXTENT_TESTING -D__POOL_EXPOSE
CLEANFILES+=	extest.c extest.out
WARNS?=		1

.PATH:		${NETBSDSRCDIR}/sys/kern

regress: ${PROG}
	./${PROG} >extest.out
	tail +4 ${.CURDIR}/extest.exp | diff - extest.out

extest.c: extest.awk tests
	${TOOL_AWK} -f ${.CURDIR}/extest.awk <${.CURDIR}/tests >extest.c

.include <bsd.prog.mk>
