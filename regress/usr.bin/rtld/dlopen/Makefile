# $NetBSD: Makefile,v 1.8 2003/06/26 18:43:35 drochner Exp $

NOMAN=  	# defined

PROG=		dlopen
CLEANFILES+=	dlopen.out

# we don't access program symbols from the shared object yet
#LDADD+=	-export-dynamic

regress:
	if [ -f ${TESTLIBDIR}/libtest.so ]; then \
		./${PROG} ${TESTLIBDIR}/libtest.so >dlopen.out; \
		tail +4 ${.CURDIR}/dlopen.exp | diff - dlopen.out; \
	fi

.include <bsd.prog.mk>

TESTLIBDIR!=	cd ${.CURDIR}/../testlib && ${PRINTOBJDIR}
