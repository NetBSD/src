# $NetBSD: Makefile,v 1.12 2011/05/26 12:56:29 joerg Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/roff/troff/Makefile,v 1.4 2003/05/01 13:22:20 ru Exp $

DIST_SUBDIR=	src/roff/troff

PROG=		troff
SRCS=		dictionary.cpp div.cpp env.cpp input.cpp \
		majorminor.cpp mtsm.cpp node.cpp number.cpp reg.cpp
CLEANFILES=	majorminor.cpp ${MAN}
USETBL=

CWARNFLAGS.clang+=	-Wno-format-security

.include <bsd.prog.mk>

majorminor.cpp: ${GROFF_DIST}/VERSION ${GROFF_DIST}/REVISION
	${_MKTARGET_CREATE}
	(\
	    maj=`${TOOL_SED} -e 's/^\([^.]*\)\..*$$/\1/' \
		${GROFF_DIST}/VERSION` ;\
	    min=`${TOOL_SED} -e 's/^[^.]*\.\([0-9]*\).*$$/\1/' \
		${GROFF_DIST}/VERSION` ;\
	    r=`cat ${GROFF_DIST}/REVISION` ;\
	    echo 'const char *major_version = "'$$maj'";' ;\
	    echo 'const char *minor_version = "'$$min'";' ;\
	    echo 'const char *revision = "'$$r'";' ;\
	) > ${.TARGET}
