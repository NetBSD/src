#	$NetBSD: Makefile,v 1.1 2003/03/04 18:00:49 thorpej Exp $

NOMAN=		# defined

PROG=		atexit

ATEXITDIR=	${.CURDIR}/../../libc/atexit
.PATH:		${ATEXITDIR}

# Note: this test relies on being dynamically linked.  You will get a
# spurious PASS for a statically linked test.
DPADD+=		${LIBPTHREAD}
LDADD+=		-lpthread

CLEANFILES+=	output

regress: ${PROG}
	./${PROG} > output
	if cmp ${ATEXITDIR}/expected output; then \
		echo PASSED; \
	else \
		echo FAILED; exit 1; \
	fi

.include <bsd.prog.mk>
