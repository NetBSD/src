# $NetBSD: Makefile,v 1.8 2006/10/09 08:32:54 mrg Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/roff/troff/Makefile,v 1.4 2003/05/01 13:22:20 ru Exp $

DIST_SUBDIR=	src/roff/troff

PROG=		troff
SRCS=		dictionary.cpp div.cpp env.cpp input.cpp \
		majorminor.cpp mtsm.cpp node.cpp number.cpp reg.cpp
DPADD+=		${LIBGROFF} ${LIBM} ${LIBSUPCXX}
LDADD+=		${LIBGROFF} -lm -lsupc++
CLEANFILES=	majorminor.cpp ${MAN}

.include <bsd.prog.mk>

majorminor.cpp: ${GROFF_DIST}/VERSION ${GROFF_DIST}/REVISION
	${_MKTARGET_CREATE}
	(\
	    maj=`sed -e 's/^\([^.]*\)\..*$$/\1/' ${GROFF_DIST}/VERSION` ;\
	    min=`sed -e 's/^[^.]*\.\([0-9]*\).*$$/\1/' ${GROFF_DIST}/VERSION` ;\
	    r=`cat ${GROFF_DIST}/REVISION` ;\
	    echo 'const char *major_version = "'$$maj'";' ;\
	    echo 'const char *minor_version = "'$$min'";' ;\
	    echo 'const char *revision = "'$$r'";' ;\
	) > ${.TARGET}
