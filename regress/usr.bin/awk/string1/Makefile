#	$NetBSD: Makefile,v 1.1 2005/01/13 12:26:12 yamt Exp $

NOMAN=  	# defined

.include <bsd.own.mk>

CLEANFILES=	result

regress:
	${AWK} -f ${.CURDIR}/test.awk > result
	@cmp ${.CURDIR}/expected result

.include <bsd.prog.mk>
