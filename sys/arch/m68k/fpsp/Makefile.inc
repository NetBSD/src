#	$NetBSD: Makefile.inc,v 1.20.16.1 2018/06/25 07:25:43 pgoyette Exp $
#
#	Configuration variables (default values are below):
#
#	S	must be set to the top of the 'sys' tree.
#	FPSPDST	may be set to the location of the directory where library
#		objects are to be built.  Defaults to ${.OBJDIR}/lib/kern.
#	FPSP_AS	isn't there. We always build an object file.
#	FPSPMISCCPPFLAGS
#		Miscellaneous cpp flags to be passed to the library's Makefile
#		when building.
#	FPSPMISCMAKEFLAGS
#		Miscellaneous flags to be passed to the library's Makefile when
#		building.  See library's Makefile for more details about
#		supported flags and their default values.

# Default values:
FPSPDST?=	${.OBJDIR}/lib/fpsp
FPSPDOTDIR?= ../../.

FPSPDIR=	${S:S@^.@${FPSPDOTDIR}@:q}/arch/m68k/fpsp
FPSPOBJ=	${FPSPDST}/fpsp.o
HAS_FPSP!=	grep '^\#define	FPSP	1$$' opt_fpsp.h || echo
.if !empty(HAS_FPSP)
FPSP=		${FPSPOBJ}
.endif

#	    CC=${CC:q} CFLAGS=${CFLAGS:q} \
#	    AS=${AS:q} AFLAGS=${AFLAGS:q} \
#	    LD=${LD:q} STRIP=${STRIP:q} \

FPSPMAKE= \
	cd ${FPSPDST} && ${MAKE} -f ${FPSPDIR:q}/Makefile \
	    FPSPDIR=${FPSPDIR:q} \
	    CC=${CC:q} CFLAGS=${CFLAGS:q} \
	    LD=${LD:q} STRIP=${STRIP:q} \
	    MACHINE=${MACHINE} \
	    MACHINE_ARCH=${MACHINE_ARCH:q} \
	    FPSPCPPFLAGS=${CPPFLAGS:S@^-I.@-I${FPSPDOTDIR}@g:q} \
	    FPSPMISCCPPFLAGS=${FPSPMISCCPPFLAGS:q} \
	    FPSPDIR=${FPSPDIR:q} \
	    ${FPSPMISCMAKEFLAGS}

${FPSP}:		.NOTMAIN __always_make_fpsp
	@echo making sure the 68040 FPSP is up to date...
	@${FPSPMAKE} fpsp.o

${FPSPLIB_PROF}:	.NOTMAIN __always_make_fpsp
	@echo making sure the profiled 68040 FPSP is up to date...
	@${FPSPMAKE} fpsp.po

clean:			.NOTMAIN cleanfpsp
cleanfpsp:		.NOTMAIN __always_make_fpsp
	@echo cleaning the 68040 FPSP objects
	@${FPSPMAKE} clean

cleandir distclean:	.NOTMAIN cleandirfpsp
cleandirfpsp:	.NOTMAIN __always_make_fpsp
# So we don't really need a cleandir target without a depend target.
# On the other hand, were "make cleandir" in the kernel build
# directory to fail to remove all the floating point objects, that   
# would tend to violate the principle of least surprise!
	@echo cleandiring the 68040 FPSP objects
	@${FPSPMAKE} clean

#depend:			.NOTMAIN dependfpsp
#dependfpsp:		.NOTMAIN __always_make_fpsp
#	@echo depending the 68040 FPSP objects
#	@${FPSPMAKE} depend
#

__always_make_fpsp:	.NOTMAIN
	@(mkdir -p ${FPSPDST})
