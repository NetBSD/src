#	$NetBSD: Makefile,v 1.1 2005/03/23 17:28:55 he Exp $

NOMAN=  	# defined

.include <bsd.own.mk>

CLEANFILES=	result

regress:
	${AWK} -f ${.CURDIR}/test.awk ${.CURDIR}/input > result
	@cmp ${.CURDIR}/expected result

.include <bsd.prog.mk>
