#	$NetBSD: Makefile.inc,v 1.1 1999/03/25 04:16:15 simonb Exp $
#
#	Configuration variables (default values are below):
#
#	S	must be set to the top of the 'sys' tree.
#	PMAXDST	may be set to the location of the directory where library
#		objects are to be built.  Defaults to ${.OBJDIR}/lib/pmax.
#	PMAXMISCCPPFLAGS
#		Miscellaneous cpp flags to be passed to the library's Makefile
#		when building.
#	PMAXMISCMAKEFLAGS
#		Miscellaneous flags to be passed to the library's Makefile when
#		building.  See library's Makefile for more details about
#		supported flags and their default values.

# Default values:
PMAXDST?=		${.OBJDIR}/lib/pmax

#PMAXDIR=		$S/arch/pmax/stand/lib
PMAXLIB=		${PMAXDST}/libpmax.a

PMAXMAKE= \
	cd ${PMAXDIR} && MAKEOBJDIR=${PMAXDST} ${MAKE} \
	    CC=${CC:Q} CFLAGS=${CFLAGS:Q} \
	    AS=${AS:Q} AFLAGS=${AFLAGS:Q} \
	    LD=${LD:Q} STRIP=${STRIP:Q} \
	    MACHINE=${MACHINE} MACHINE_ARCH=${MACHINE_ARCH:Q} \
	    PMAXCPPFLAGS=${CPPFLAGS:S@^-I.@-I../../.@g:Q} \
	    PMAXMISCCPPFLAGS=${PMAXMISCCPPFLAGS:Q} S=${S:Q} \
	    ${PMAXMISCMAKEFLAGS}

${PMAXLIB}:		.NOTMAIN __always_make_pmaxlib
	@echo making sure the pmax library is up to date...
	@${PMAXMAKE} libpmax.a

clean:			.NOTMAIN cleanpmaxlib
cleanpmaxlib:		.NOTMAIN __always_make_pmaxlib
	@echo cleaning the pmax library objects
	@${PMAXMAKE} clean

cleandir distclean:	.NOTMAIN cleandirpmaxlib
cleandirpmaxlib:		.NOTMAIN __always_make_pmaxlib
	@echo cleandiring the pmax library objects
	@${PMAXMAKE} cleandir

depend:			.NOTMAIN dependpmaxlib
dependpmaxlib:		.NOTMAIN __always_make_pmaxlib
	@echo depending the pmax library objects
	@${PMAXMAKE} depend

__always_make_pmaxlib:	.NOTMAIN
	@mkdir -p ${PMAXDST}
