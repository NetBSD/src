# $NetBSD: Makefile,v 1.7 2003/02/01 18:44:06 kleink Exp $

NOMAN=  	# defined

PROG=		dlopen
CLEANFILES+=	dlopen.out

# XXX work around toolchain problems (shared libs not linked against libgcc,
# should be fixed in gcc3, providing a shared libgcc)
#LDADD=	-Wl,-whole-archive -lgcc -Wl,-no-whole-archive
#LDADD+=	-export-dynamic

regress:
	if [ -f ${TESTLIBDIR}/libtest.so ]; then \
		./${PROG} ${TESTLIBDIR}/libtest.so >dlopen.out; \
		tail +4 ${.CURDIR}/dlopen.exp | diff - dlopen.out; \
	fi

.include <bsd.prog.mk>

TESTLIBDIR!=	cd ${.CURDIR}/../testlib && ${PRINTOBJDIR}
