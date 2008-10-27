#	$NetBSD: Makefile,v 1.2 2008/10/27 20:59:22 apb Exp $

NOMAN=  	# defined

.include <bsd.own.mk>

AWK?=	${TOOL_AWK}

CLEANFILES=	result

regress:
	${AWK} -f ${.CURDIR}/test.awk < input > result
	@cmp ${.CURDIR}/expected result

.include <bsd.prog.mk>
