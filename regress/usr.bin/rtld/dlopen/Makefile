# $NetBSD: Makefile,v 1.1 2000/12/08 19:22:51 drochner Exp $

MKMAN=  no

PROG=	dlopen

TESTLIBDIR!=	cd ${.CURDIR}/../testlib; make print-objdir

regress:
	if [ -f ${TESTLIBDIR}/testlib.so ]; then \
		${.OBJDIR}/dlopen ${TESTLIBDIR}/testlib.so >dlopen.out; \
		tail +4 ${.CURDIR}/dlopen.exp | diff - dlopen.out; \
	fi

CLEANFILES+=	dlopen.out

.include <bsd.prog.mk>
