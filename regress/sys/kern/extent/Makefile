# $NetBSD: Makefile,v 1.5 2002/09/18 04:16:02 lukem Exp $

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
	awk -f ${.CURDIR}/extest.awk <${.CURDIR}/tests >extest.c

.include <bsd.prog.mk>
