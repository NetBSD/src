#	$NetBSD: Makefile,v 1.1 2004/10/03 12:35:15 yamt Exp $

NOMAN=	# defined
WARNS=	3

PROG=	getdents

TESTDIR= test

clean:
	rm -rf ${TESTDIR}

regress: ${PROG}
	@set -e;					\
	test -d ${TESTDIR} || mkdir ${TESTDIR};		\
	for x in $$(jot 1000); do			\
		touch ${TESTDIR}/XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX$$x; \
	done;						\
	if ./${PROG} ${TESTDIR} > /dev/null; then	\
		echo PASSED; exit 0;			\
	else						\
		echo FAILED; exit 1;			\
	fi

.include <bsd.prog.mk>
