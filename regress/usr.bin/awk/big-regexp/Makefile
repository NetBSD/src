#	$NetBSD: Makefile,v 1.1 2006/07/04 03:02:21 christos Exp $

NOMAN=  	# defined

.include <bsd.own.mk>

CLEANFILES=	result

regress:
	${AWK} -f ${.CURDIR}/test.awk < input > result
	@cmp ${.CURDIR}/expected result

.include <bsd.prog.mk>
