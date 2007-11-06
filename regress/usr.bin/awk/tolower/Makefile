#	$NetBSD: Makefile,v 1.1.2.2 2007/11/06 23:12:21 matt Exp $

NOMAN=  	# defined

.include <bsd.own.mk>

CLEANFILES=	result

regress:
	env LANG=en_US.UTF-8 \
		${AWK} -f ${.CURDIR}/test.awk ${.CURDIR}/input > result
	@cmp ${.CURDIR}/expected result

.include <bsd.prog.mk>
