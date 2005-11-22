#	$NetBSD: Makefile,v 1.1 2005/11/22 12:18:43 yamt Exp $

NOMAN=	# defined
WARNS=	3

PROG=	posix_fadvise

TESTFILE=	TESTFILE
CLEANFILES=	${TESTFILE}

regress: ${PROG}
	@set -e;					\
	touch ${TESTFILE};				\
	if ./${PROG} < ${TESTFILE}; then		\
		echo PASSED; exit 0;			\
	else						\
		echo FAILED; exit 1;			\
	fi

.include <bsd.prog.mk>
