#	$NetBSD: Makefile,v 1.1 2002/08/02 19:10:50 perseant Exp $

PROG=	9994_f
NOMAN=	# defined
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
