# $NetBSD: Makefile,v 1.2 2001/10/31 15:57:56 tv Exp $

MKMAN=  no

PROG=	dlopen

TESTLIBDIR!=	cd ${.CURDIR}/../testlib; make print-objdir

regress:
	if [ -f ${TESTLIBDIR}/testlib.so ]; then \
		./${PROG} ${TESTLIBDIR}/testlib.so >dlopen.out; \
		tail +4 ${.CURDIR}/dlopen.exp | diff - dlopen.out; \
	fi

CLEANFILES+=	dlopen.out

.include <bsd.prog.mk>
