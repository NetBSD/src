# $NetBSD: Makefile,v 1.6 2004/05/23 02:24:51 lukem Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/libs/libgroff/Makefile,v 1.5 2003/05/01 13:22:19 ru Exp $

DIST_SUBDIR=	src/libs/libgroff

MKPRIVATELIB=	yes

LIB=		groff
SRCS=		assert.cpp change_lf.cpp cmap.cpp color.cpp cset.cpp device.cpp errarg.cpp\
		error.cpp fatal.cpp filename.cpp font.cpp fontfile.cpp geometry.cpp\
		htmlhint.cpp invalid.cpp lf.cpp lineno.cpp macropath.cpp\
		maxfilename.cpp mksdir.cpp nametoindex.cpp new.cpp paper.cpp prime.cpp\
	 	progname.cpp ptable.cpp searchpath.cpp string.cpp strsave.cpp\
		tmpfile.cpp tmpname.cpp version.cpp
SRCS+=		getopt.c getopt1.c iftoa.c itoa.c matherr.c
CLEANFILES=	version.cpp

.include <bsd.lib.mk>

version.cpp: ${GROFF_DIST}/VERSION ${GROFF_DIST}/REVISION
	${_MKTARGET_CREATE}
	(\
	    v=`cat ${GROFF_DIST}/VERSION`;\
	    r=`cat ${GROFF_DIST}/REVISION`;\
	    echo 'const char *version_string = "'$$v'";' ;\
	    echo 'const char *revision_string = "'$$r'";' ;\
	    echo 'extern "C" const char *Version_string = "'$$v'.'$$r'";' |\
		sed -e 's/\.0\"/\"/'; \
	) > ${.TARGET}
