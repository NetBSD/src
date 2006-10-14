#	$NetBSD: Makefile,v 1.2 2006/10/14 20:57:10 bjh21 Exp $

NOMAN=		# defined

PROG=		loan1
TESTFILE=	XXX
DEVZERO=	/dev/zero
CLEANFILES+=	${TESTFILE}

regress: ${PROG}
	@dd if=/dev/zero of=${TESTFILE} bs=1024 seek=256 count=1
	@if ./loan1 ${TESTFILE}; then \
		echo "PASSED1"; \
	else \
		echo "FAILED1"; \
	fi
	@if ./loan1 ${DEVZERO}; then \
		echo "PASSED2"; \
	else \
		echo "FAILED2"; \
	fi

.include <bsd.prog.mk>
