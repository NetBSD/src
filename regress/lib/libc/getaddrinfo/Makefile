#	$NetBSD: Makefile,v 1.3 2003/09/30 04:20:24 mycroft Exp $

NOMAN=		# defined

PROG=		gaitest
SRCS=		gaitest.c
CLEANFILES+=	out

regress: ${PROG}
	sh ${.CURDIR}/testsuite.sh >out 2>&1
	@if diff -u ${.CURDIR}/answer out; then \
		echo SUCCESS; \
	else \
		echo FAIL; \
		exit 1; \
	fi

.include <bsd.prog.mk>
