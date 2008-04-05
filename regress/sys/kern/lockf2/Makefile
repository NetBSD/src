#	$NetBSD: Makefile,v 1.1 2008/04/05 03:34:47 yamt Exp $

NOMAN=	# defined
WARNS=	3

PROG=	lockf2

DBG=-g
TESTFILE=	TESTFILE
CLEANFILES=	${TESTFILE}

regress: ${PROG}
	@set -e;					\
	if ./${PROG} > ${TESTFILE}; then		\
		echo PASSED; exit 0;			\
	else						\
		echo FAILED; exit 1;			\
	fi

.include <bsd.prog.mk>
