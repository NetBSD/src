#	$NetBSD: Makefile,v 1.3 2006/09/21 17:33:08 reinoud Exp $

NOMAN=	# defined

PROG=	lseek

clean:
	rm -f test-file

regress: ${PROG}
	@set -e; \
	dd if=/dev/zero of=test-file bs=2k count=1000 > /dev/null 2>&1; \
	if ./${PROG} test-file > /dev/null; then \
		echo PASSED; exit 0; \
	else \
		echo FAILED; exit 1; \
	fi

.include <bsd.prog.mk>
