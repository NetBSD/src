# $NetBSD: Makefile,v 1.4 2001/12/12 01:24:18 tv Exp $

PROG=		dlopen
NOMAN=  	# defined
CLEANFILES+=	dlopen.out

regress:
	if [ -f ${TESTLIBDIR}/testlib.so ]; then \
		./${PROG} ${TESTLIBDIR}/testlib.so >dlopen.out; \
		tail +4 ${.CURDIR}/dlopen.exp | diff - dlopen.out; \
	fi

.include <bsd.prog.mk>

TESTLIBDIR!=	cd ${.CURDIR}/../testlib && ${PRINTOBJDIR}
