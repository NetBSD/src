#	$NetBSD: Makefile.inc,v 1.32 2000/01/19 16:30:05 tsutsui Exp $
#
#	Configuration variables (default values are below):
#
#	S	must be set to the top of the 'sys' tree.
#	KERNDST	may be set to the location of the directory where library
#		objects are to be built.  Defaults to ${.OBJDIR}/lib/kern.
#	KERN_AS	may be set to 'obj' to build a object from the library's
#		object files.  (Otherwise, a library will be built.)
#		Defaults to 'library'.
#	KERNMISCCPPFLAGS
#		Miscellaneous cpp flags to be passed to the library's Makefile
#		when building.
#	KERNMISCMAKEFLAGS
#		Miscellaneous flags to be passed to the library's Makefile when
#		building.  See library's Makefile for more details about
#		supported flags and their default values.

# Default values:
KERNDST?=	${.OBJDIR}/lib/kern
KERN_AS?=	library
KERNDOTDIR?= ../../.

KERNDIR=	${S:S@^.@${KERNDOTDIR}@:Q}/lib/libkern
.if (${KERN_AS} == "obj")
KERNLIB=	${KERNDST}/libkern.o
KERNLIB_PROF=	${KERNDST}/libkern.po
.else
KERNLIB=	${KERNDST}/libkern.a
KERNLIB_PROF=	${KERNDST}/libkern_p.a
.endif

KERNMAKE= \
	cd ${KERNDST} && ${MAKE} -f ${KERNDIR:Q}/Makefile \
	    KERNDIR=${KERNDIR:Q} \
	    CC=${CC:Q} CFLAGS=${CFLAGS:Q} \
	    AS=${AS:Q} AFLAGS=${AFLAGS:Q} \
	    LORDER=${LORDER:Q} \
	    TSORT=${TSORT:Q} \
	    LD=${LD:Q} STRIPPROG=${STRIPPROG:Q} \
	    AR=${AR:Q} NM=${NM:Q} \
	    RANLIB=${RANLIB:Q} SIZE=${SIZE:Q} \
	    MACHINE=${MACHINE} MACHINE_ARCH=${MACHINE_ARCH:Q} \
	    KERNCPPFLAGS=${CPPFLAGS:S@^-I.@-I${KERNDOTDIR}@g:Q} \
	    KERNMISCCPPFLAGS=${KERNMISCCPPFLAGS:Q} \
	    ${KERNMISCMAKEFLAGS}

${KERNLIB}:		.NOTMAIN __always_make_kernlib
	@echo making sure the kern library is up to date...
.if (${KERN_AS} == "library")
	@${KERNMAKE} libkern.a
.else
	@${KERNMAKE} libkern.o
.endif

${KERNLIB_PROF}:	.NOTMAIN __always_make_kernlib
	@echo making sure the profiled kern library is up to date...
.if (${KERN_AS} == "library")
	@${KERNMAKE} libkern_p.a
.else
	@${KERNMAKE} libkern.po
.endif

clean:			.NOTMAIN cleankernlib
cleankernlib:		.NOTMAIN __always_make_kernlib
	@echo cleaning the kern library objects
	@${KERNMAKE} clean

cleandir distclean:	.NOTMAIN cleandirkernlib
cleandirkernlib:	.NOTMAIN __always_make_kernlib
	@echo cleandiring the kern library objects
	@${KERNMAKE} cleandir

depend:			.NOTMAIN dependkernlib
dependkernlib:		.NOTMAIN __always_make_kernlib
	@echo depending the kern library objects
	@${KERNMAKE} depend

__always_make_kernlib:	.NOTMAIN
	@(mkdir -p ${KERNDST})

.PHONY: __always_make_kernlib
.PHONY: cleankernlib cleandirkernlib dependkernlib
