#	$NetBSD: Makefile.inc,v 1.13 1997/11/04 21:18:28 gwr Exp $
#
#	Configuration variables (default values are below):
#
#	S	must be set to the top of the 'sys' tree.
#	COMPATDST
#		may be set to the location of the directory where library
#		objects are to be built.  Defaults to ${.OBJDIR}/lib/compat.
#	COMPAT_AS
#		may be set to 'obj' to build a object from the library's
#		object files.  (Otherwise, a library will be built.)
#		Defaults to 'library'.
#	COMPATMISCMAKEFLAGS
#		Miscellaneous flags to be passed to the library's Makefile when
#		building.  See library's Makefile for more details about
#		supported flags and their default values.

# Default values:
COMPATDST?=		${.OBJDIR}/lib/compat
COMPAT_AS?=		library

COMPATDIR=		$S/compat/common
.if (${COMPAT_AS} == "obj")
COMPATLIB=		${COMPATDST}/libcompat.o
COMPATLIB_PROF=	${COMPATDST}/libcompat.po
.else
COMPATLIB=		${COMPATDST}/libcompat.a
COMPATLIB_PROF=	${COMPATDST}/libcompat_p.a
.endif

COMPATMAKE= \
	cd ${COMPATDIR} && MAKEOBJDIR=${COMPATDST} ${MAKE} \
	    CC=${CC:Q} CFLAGS=${CFLAGS:Q} \
	    AS=${AS:Q} AFLAGS=${AFLAGS:Q} \
	    AR=${AR:Q} NM=${NM:Q} \
	    LORDER=${LORDER:Q} \
	    TSORT=${TSORT:Q} \
	    RANLIB=${RANLIB:Q} \
	    LD=${LD:Q} STRIP=${STRIP:Q} \
	    MACHINE=${MACHINE} MACHINE_ARCH=${MACHINE_ARCH:Q} \
	    COMPATCPPFLAGS=${CPPFLAGS:S@^-I.@-I../../.@g:Q} \
	    ${COMPATMISCMAKEFLAGS}

${COMPATLIB}:		.NOTMAIN __always_make_compatlib
	@echo making sure the compat library is up to date...
.if (${COMPAT_AS} == "library")
	@${COMPATMAKE} libcompat.a
.else
	@${COMPATMAKE} libcompat.o
.endif

${COMPATLIB_PROF}:	.NOTMAIN __always_make_compatlib
	@echo making sure the profiled compat library is up to date...
.if (${COMPAT_AS} == "library")
	@${COMPATMAKE} libcompat_p.a
.else
	@${COMPATMAKE} libcompat.po
.endif

clean:			.NOTMAIN cleancompatlib
cleancompatlib:		.NOTMAIN __always_make_compatlib
	@echo cleaning the compat library objects
	@${COMPATMAKE} clean

cleandir:		.NOTMAIN cleandircompatlib
cleandircompatlib:	.NOTMAIN __always_make_compatlib
	@echo cleandiring the compat library objects
	@${COMPATMAKE} cleandir

depend:			.NOTMAIN dependcompatlib
dependcompatlib:	.NOTMAIN __always_make_compatlib
	@echo depending the compat library objects
	@${COMPATMAKE} depend

__always_make_compatlib: .NOTMAIN
	@mkdir -p ${COMPATDST}
