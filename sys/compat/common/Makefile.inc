#	$NetBSD: Makefile.inc,v 1.15.2.2 2000/12/08 09:08:09 bouyer Exp $
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
COMPATDOTDIR?= ../../.

COMPATDIR=		${S:S@^.@${COMPATDOTDIR}@:Q}/compat/common
.if (${COMPAT_AS} == "obj")
COMPATLIB=		${COMPATDST}/libcompat.o
COMPATLIB_PROF=	${COMPATDST}/libcompat.po
.else
COMPATLIB=		${COMPATDST}/libcompat.a
COMPATLIB_PROF=	${COMPATDST}/libcompat_p.a
.endif

COMPATMAKE= \
	cd ${COMPATDST} && ${MAKE} -f ${COMPATDIR:Q}/Makefile \
	    COMPATDIR=${COMPATDIR:Q} \
	    CC=${CC:Q} CFLAGS=${CFLAGS:Q} \
	    AS=${AS:Q} AFLAGS=${AFLAGS:Q} \
	    AR=${AR:Q} NM=${NM:Q} \
	    LORDER=${LORDER:Q} \
	    TSORT=${TSORT:Q} \
	    RANLIB=${RANLIB:Q} \
	    LD=${LD:Q} STRIP=${STRIP:Q} \
	    MACHINE=${MACHINE} MACHINE_ARCH=${MACHINE_ARCH:Q} \
	    COMPATCPPFLAGS=${CPPFLAGS:S@^-I.@-I${COMPATDOTDIR}@g:Q} \
	    ${COMPATMISCMAKEFLAGS}

${COMPATLIB}:		.NOTMAIN .MAKE __always_make_compatlib
	@echo making sure the compat library is up to date...
.if (${COMPAT_AS} == "library")
	@${COMPATMAKE} libcompat.a
.else
	@${COMPATMAKE} libcompat.o
.endif

${COMPATLIB_PROF}:	.NOTMAIN .MAKE __always_make_compatlib
	@echo making sure the profiled compat library is up to date...
.if (${COMPAT_AS} == "library")
	@${COMPATMAKE} libcompat_p.a
.else
	@${COMPATMAKE} libcompat.po
.endif

clean:			.NOTMAIN cleancompatlib
cleancompatlib:		.NOTMAIN .MAKE __always_make_compatlib
	@echo cleaning the compat library objects
	@${COMPATMAKE} clean

cleandir distclean:	.NOTMAIN cleandircompatlib
cleandircompatlib:	.NOTMAIN .MAKE  __always_make_compatlib
	@echo cleandiring the compat library objects
	@${COMPATMAKE} cleandir

depend:			.NOTMAIN dependcompatlib
dependcompatlib:	.NOTMAIN .MAKE __always_make_compatlib
	@echo depending the compat library objects
	@${COMPATMAKE} depend

__always_make_compatlib: .NOTMAIN
	@mkdir -p ${COMPATDST}

.PHONY: __always_make_compatlib
.PHONY: cleancompatlib cleandircompatlib dependcompatlib
