# $NetBSD: Makefile,v 1.6 2004/07/30 15:04:19 wiz Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/roff/troff/Makefile,v 1.4 2003/05/01 13:22:20 ru Exp $

DIST_SUBDIR=	src/roff/troff

PROG_CXX=	troff
SRCS=		dictionary.cpp div.cpp env.cpp input.cpp \
		majorminor.cpp node.cpp number.cpp reg.cpp
DPADD+=		${LIBGROFF} ${LIBM}
LDADD+=		${LIBGROFF} -lm
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
