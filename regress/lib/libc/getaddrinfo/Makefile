#	$NetBSD: Makefile,v 1.2 2002/09/18 05:41:38 lukem Exp $

NOMAN=		# defined

PROG=		gaitest
SRCS=		gaitest.c
CLEANFILES+=	out

regress: ${PROG}
	sh ${.CURDIR}/testsuite.sh >out 2>&1
	@if diff -u out ${.CURDIR}/answer; then \
		echo SUCCESS; \
	else \
		echo FAIL; \
		exit 1; \
	fi

.include <bsd.prog.mk>
