#	$NetBSD: Makefile.inc,v 1.7 1996/09/09 18:42:51 mycroft Exp $
#
#	NOTE: $S must correspond to the top of the 'sys' tree

APMDIR=		${I386}/apm_init

APMDST?=	${.OBJDIR}/lib/apm_init
APMINC?=	${APMDST}/biostramp.inc

APMDEPS= \
	${APMDIR}/Makefile \
	${APMDIR}/bin2asm.sh \
	${APMDIR}/biostramp.S \
	${APMDIR}/rmaouthdr \
	${.OBJDIR}/assym.h \
	${.OBJDIR}/apm.h \
	${I386}/include/param.h \
	${I386}/include/cpu.h \
	${I386}/include/psl.h \
	${I386}/include/intr.h \
	${I386}/include/frame.h \
	${I386}/include/signal.h \
	${I386}/include/trap.h \
	${I386}/include/segments.h \
	${I386}/include/cputypes.h \
	${I386}/include/specialreg.h \
	${I386}/include/apmvar.h \
	${I386}/include/asm.h

APMMAKE= \
	cd ${APMDIR} && MAKEOBJDIR=${APMDST} ${MAKE} \
	    CC='${CC}' CFLAGS='${CFLAGS}' \
	    AS='${AS}' AFLAGS='${AFLAGS}' \
	    LD='${LD}' STRIP='${STRIP}' \
	    APMCPPFLAGS='${CPPFLAGS:S@^-I.$@-I${.OBJDIR}@}' \
	    APMDEPS='${APMDEPS}'

${APMINC}:		.NOTMAIN __always_make_apmlib \
			${APMDEPS} apm.h
	@echo making sure the apm grappling hook is up to date...
	@${APMMAKE} biostramp.inc

clean:: 		.NOTMAIN __always_make_apmlib
	@echo cleaning the apm grappling hook objects
	@${APMMAKE} clean

depend:: 		.NOTMAIN __always_make_apmlib
	@echo depending the apm grappling hook objects
	@${APMMAKE} depend

__always_make_apmlib:		.NOTMAIN
	@mkdir -p ${APMDST}
