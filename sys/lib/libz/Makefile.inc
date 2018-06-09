#	$NetBSD: Makefile.inc,v 1.16.10.1 2018/06/09 15:35:38 martin Exp $
#
#	Configuration variables (default values are below):
#
#	S	must be set to the top of the 'sys' tree.
#	ZDST	may be set to the location of the directory where library
#		objects are to be built.  Defaults to ${.OBJDIR}/lib/z.
#	Z_AS	may be set to 'obj' to build a object from the library's
#		object files.  (Otherwise, a library will be built.)
#		Defaults to 'library'.
#	ZMISCCPPFLAGS
#		Miscellaneous cpp flags to be passed to the library's Makefile
#		when building.
#	ZMISCMAKEFLAGS
#		Miscellaneous flags to be passed to the library's Makefile when
#		building.  See library's Makefile for more details about
#		supported flags and their default values.

# Default values:
ZDST?=		${.OBJDIR}/lib/z
Z_AS?=		library
ZDOTDIR?= ../../.

ZDIR=		${S:S@^.@${ZDOTDIR}@:q}/lib/libz
.if (${Z_AS} == "obj")
ZLIB=		${ZDST}/libz.o
ZLIB_PROF=	${ZDST}/libz.po
.else
ZLIB=		${ZDST}/libz.a
ZLIB_PROF=	${ZDST}/libz_p.a
.endif

ZMAKE= \
	cd ${ZDST} && ${MAKE} -f ${ZDIR:q}/Makefile \
	    ZDIR=${ZDIR:q} \
	    CC=${CC:q} CFLAGS=${CFLAGS:q} CPUFLAGS= \
	    AS=${AS:q} AFLAGS=${AFLAGS:q} \
	    LORDER=${LORDER:q} \
	    TSORT=${TSORT:q} \
	    LD=${LD:q} STRIP=${STRIP:q} \
	    AR=${AR:q} NM=${NM:q} \
	    RANLIB=${RANLIB:q} SIZE=${SIZE:q} \
	    MACHINE=${MACHINE} MACHINE_ARCH=${MACHINE_ARCH:q} \
	    ZCPPFLAGS=${CPPFLAGS:S@^-I.@-I${ZDOTDIR}@g:q} \
	    ZMISCCPPFLAGS=${ZMISCCPPFLAGS:q} \
	    ${ZMISCMAKEFLAGS}

${ZLIB}:		.NOTMAIN .MAKE __always_make_zlib
	@echo making sure the z library is up to date...
.if (${Z_AS} == "library")
	@${ZMAKE} libz.a
.else
	@${ZMAKE} libz.o
.endif

${ZLIB_PROF}:		.NOTMAIN .MAKE __always_make_zlib
	@echo making sure the profiled z library is up to date...
.if (${Z_AS} == "library")
	@${ZMAKE} libz_p.a
.else
	@${ZMAKE} libz.po
.endif

clean:			.NOTMAIN cleanzlib
cleanzlib:		.NOTMAIN
	@echo cleaning the z library objects
	@if [ -d "${ZDST}" ]; then ${ZMAKE} clean; fi

cleandir distclean:	.NOTMAIN cleandirzlib
cleandirzlib:		.NOTMAIN
	@echo cleandiring the z library objects
	@if [ -d "${ZDST}" ]; then ${ZMAKE} cleandir; fi

dependall depend:	.NOTMAIN dependzlib
dependzlib:		.NOTMAIN .MAKE __always_make_zlib
	@echo depending the z library objects
	@${ZMAKE} depend

__always_make_zlib:	.NOTMAIN
	@mkdir -p ${ZDST}

.PHONY: __always_make_zlib
.PHONY: cleanzlib cleandirzlib dependzlib
