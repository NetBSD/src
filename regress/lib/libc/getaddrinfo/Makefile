#	$NetBSD: Makefile,v 1.4 2003/10/26 07:25:34 lukem Exp $

NOMAN=		# defined

PROG=		gaitest
SRCS=		gaitest.c
CLEANFILES+=	out

regress: ${PROG}
	${HOST_SH} ${.CURDIR}/testsuite.sh >out 2>&1
	@if diff -u ${.CURDIR}/answer out; then \
		echo SUCCESS; \
	else \
		echo FAIL; \
		exit 1; \
	fi

.include <bsd.prog.mk>
