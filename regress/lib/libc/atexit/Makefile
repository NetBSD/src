#	$NetBSD: Makefile,v 1.1 2003/03/01 04:20:54 thorpej Exp $

NOMAN=		# defined

PROG=		atexit

CLEANFILES+=	output

regress: ${PROG}
	./${PROG} > output
	if cmp ${.CURDIR}/expected output; then \
		echo PASSED; \
	else \
		echo FAILED; exit 1; \
	fi

.include <bsd.prog.mk>
