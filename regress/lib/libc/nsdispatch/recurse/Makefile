#	$NetBSD: Makefile,v 1.1 2004/08/02 00:16:25 thorpej Exp $

NOMAN=	# defined
PROG=	recurse
DPADD+=	${LIBPTHREAD}
LDADD+=	-lpthread

CLEANFILES+= output

regress: ${PROG}
	./${PROG} > output
	if cmp ${.CURDIR}/expected output; then \
		echo PASSED; \
	else \
		echo FAILED; exit 1; \
	fi

.include <bsd.prog.mk>
