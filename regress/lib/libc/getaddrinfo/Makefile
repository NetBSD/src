#	$NetBSD: Makefile,v 1.5 2006/03/16 16:24:23 rpaulo Exp $

NOMAN=		# defined

PROG=		gaitest
SRCS=		gaitest.c
CLEANFILES+=	out

# XXX: no IPv6-only support yet
regress: ${PROG}
	${HOST_SH} ${.CURDIR}/testsuite.sh >out 2>&1 
	@if ifconfig lo0 inet6 > /dev/null 2>&1; then \
		if diff -u ${.CURDIR}/answer-v4v6 out; then \
			echo SUCCESS; \
		else \
			echo FAIL; \
			exit 1; \
		fi \
	else \
		if diff -u ${.CURDIR}/answer-v4 out; then \
			echo SUCCESS; \
		else \
			echo FAIL; \
			exit 1; \
		fi \
	fi

.include <bsd.prog.mk>
