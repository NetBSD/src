#	$NetBSD: Makefile,v 1.2 2002/09/18 05:41:52 lukem Exp $

NOMAN=	# defined

PROG=	9994_f
TMP?=	/tmp
TESTFS?=.

regress: ${PROG}
	@cp ${PROG} ${TESTFS}
	@cd ${TESTFS} && ./${PROG} -c ${TMP} -n 1000 -q || (echo 'test failed' && exit 1)
	@diff -q ${TESTFS}/test ${TMP}/control || (echo 'incorrect results' && exit 1)
	@rm -f ${TESTFS}/test ${TMP}/control ${TESTFS}/${PROG}

clean:
	rm -f test control

.include <bsd.prog.mk>
