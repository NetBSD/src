# $NetBSD: Makefile,v 1.5 2002/08/28 17:09:17 drochner Exp $

PROG=		dlopen
NOMAN=  	# defined
CLEANFILES+=	dlopen.out

# XXX work around toolchain problems (shared libs not linked against libgcc,
# should be fixed in gcc3, providing a shared libgcc)
#LDADD=	-Wl,-whole-archive -lgcc -Wl,-no-whole-archive
#LDADD+=	-export-dynamic

regress:
	if [ -f ${TESTLIBDIR}/testlib.so ]; then \
		./${PROG} ${TESTLIBDIR}/testlib.so >dlopen.out; \
		tail +4 ${.CURDIR}/dlopen.exp | diff - dlopen.out; \
	fi

.include <bsd.prog.mk>

TESTLIBDIR!=	cd ${.CURDIR}/../testlib && ${PRINTOBJDIR}
