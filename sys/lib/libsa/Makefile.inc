#	$NetBSD: Makefile.inc,v 1.10 1998/02/07 00:11:31 cgd Exp $
#
#	Configuration variables (default values are below):
#
#	S	must be set to the top of the 'sys' tree.
#	SADST	may be set to the location of the directory where library
#		objects are to be built.  Defaults to ${.OBJDIR}/lib/sa.
#	SA_AS	may be set to 'obj' to build a object from the library's
#		object files.  (Otherwise, a library will be built.)
#		Defaults to 'library'.
#	SAMISCCPPFLAGS
#		Miscellaneous cpp flags to be passed to the library's Makefile
#		when building.
#	SAMISCMAKEFLAGS
#		Miscellaneous flags to be passed to the library's Makefile when
#		building.  See library's Makefile for more details about
#		supported flags and their default values.

# Default values:
SADST?=		${.OBJDIR}/lib/sa
SA_AS?=		library

SADIR=		$S/lib/libsa
.if (${SA_AS} == "obj")
SALIB=		${SADST}/libsa.o
SALIB_PROF=	${SADST}/libsa.po
.else
SALIB=		${SADST}/libsa.a
SALIB_PROF=	${SADST}/libsa_p.a
.endif

SAMAKE= \
	cd ${SADIR} && MAKEOBJDIR=${SADST} ${MAKE} \
	    CC=${CC:Q} CFLAGS=${CFLAGS:Q} \
	    AS=${AS:Q} AFLAGS=${AFLAGS:Q} \
	    LORDER=${LORDER:Q} \
	    TSORT=${TSORT:Q} \
	    LD=${LD:Q} STRIP=${STRIP:Q} \
	    AR=${AR:Q} NM=${NM:Q} \
	    RANLIB=${RANLIB:Q} SIZE=${SIZE:Q} \
	    MACHINE=${MACHINE} MACHINE_ARCH=${MACHINE_ARCH:Q} \
	    SACPPFLAGS=${CPPFLAGS:S@^-I.@-I../../.@g:Q} \
	    SAMISCCPPFLAGS=${SAMISCCPPFLAGS:Q} \
	    ${SAMISCMAKEFLAGS}

${SALIB}:		.NOTMAIN __always_make_salib
	@echo making sure the sa library is up to date...
.if (${SA_AS} == "library")
	@${SAMAKE} libsa.a
.else
	@${SAMAKE} libsa.o
.endif

${SALIB_PROF}:		.NOTMAIN __always_make_salib
	@echo making sure the profiled sa library is up to date...
.if (${SA_AS} == "library")
	@${SAMAKE} libsa_p.a
.else
	@${SAMAKE} libsa.po
.endif

clean:			.NOTMAIN cleansalib
cleansalib:		.NOTMAIN __always_make_salib
	@echo cleaning the sa library objects
	@${SAMAKE} clean

cleandir:		.NOTMAIN cleandirsalib
cleandirsalib:		.NOTMAIN __always_make_salib
	@echo cleandiring the sa library objects
	@${SAMAKE} cleandir

depend:			.NOTMAIN dependsalib
dependsalib:		.NOTMAIN __always_make_salib
	@echo depending the sa library objects
	@${SAMAKE} depend

__always_make_salib:	.NOTMAIN
	@mkdir -p ${SADST}
