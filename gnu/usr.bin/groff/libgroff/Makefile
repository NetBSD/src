#	$NetBSD: Makefile,v 1.15 2002/09/19 03:09:41 lukem Exp $

NOMAN=		# defined
NOPROFILE=	# defined
NOPIC=		# defined

.include "../Makefile.comp"

DIST=		${IDIST}/src/libs/libgroff
.PATH:		${DIST}

LIB=		groff
SRCS=		assert.cc change_lf.cc cmap.cc cset.cc device.cc errarg.cc \
		error.cc fatal.cc filename.cc font.cc fontfile.cc \
		htmlindicate.cc iftoa.c illegal.cc itoa.c lf.cc \
		lineno.cc macropath.cc matherr.c nametoindex.cc new.cc \
		prime.cc progname.cc ptable.cc searchpath.cc string.cc \
		strsave.cc tmpfile.cc version.cc

CLEANFILES+=	version.cc

version=`cat ${IDIST}/VERSION`
revision=`cat ${IDIST}/REVISION`

version.cc: ${IDIST}/VERSION ${IDIST}/REVISION
	@echo Making version.cc
	@echo const char \*version_string = \"$(version)\"\; >$@
	@echo const char \*revision_string = \"$(revision)\"\; >>$@
	@echo const char \*Version_string = \"$(version).$(revision)\"\; | \
		sed -e 's/\.0\"/\"/' >>$@

# only needed during build
libinstall::

.include <bsd.lib.mk>
