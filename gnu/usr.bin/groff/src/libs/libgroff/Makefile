# $NetBSD: Makefile,v 1.8.18.1 2008/05/18 12:30:08 yamt Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/libs/libgroff/Makefile,v 1.5 2003/05/01 13:22:19 ru Exp $

DIST_SUBDIR=	src/libs/libgroff

LIBISPRIVATE=	yes

LIB=		groff
SRCS=		assert.cpp change_lf.cpp cmap.cpp color.cpp cset.cpp \
		device.cpp errarg.cpp error.cpp \
		fatal.cpp filename.cpp font.cpp fontfile.cpp geometry.cpp\
		glyphuni.cpp htmlhint.cpp hypot.cpp invalid.cpp \
		lf.cpp lineno.cpp macropath.cpp \
		maxfilename.cpp maxpathname.cpp mksdir.cpp \
		nametoindex.cpp new.cpp \
		paper.cpp prime.cpp ptable.cpp quotearg.c \
		relocate.cpp searchpath.cpp spawnvp.c string.cpp \
		strsave.cpp symbol.cpp tmpfile.cpp tmpname.cpp \
		unicode.cpp uniglyph.cpp uniuni.cpp version.cpp
SRCS+=		getopt.c getopt1.c iftoa.c itoa.c matherr.c progname.c
CFLAGS+=	-D__GETOPT_PREFIX=groff_
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
