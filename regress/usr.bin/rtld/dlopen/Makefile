# $NetBSD: Makefile,v 1.3 2001/11/14 00:41:52 tv Exp $

MKMAN=  no

PROG=	dlopen

regress:
	if [ -f ${TESTLIBDIR}/testlib.so ]; then \
		./${PROG} ${TESTLIBDIR}/testlib.so >dlopen.out; \
		tail +4 ${.CURDIR}/dlopen.exp | diff - dlopen.out; \
	fi

CLEANFILES+=	dlopen.out

.include <bsd.prog.mk>

TESTLIBDIR!=	cd ${.CURDIR}/../testlib && ${PRINTOBJDIR}
