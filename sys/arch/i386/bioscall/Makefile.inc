#	$NetBSD: Makefile.inc,v 1.1 1996/08/25 23:39:15 jtk Exp $
#
#	NOTE: $S must correspond to the top of the 'sys' tree

APMDIR=	${I386}/apm_init

APMDST=	lib/apm_init
APMINC?=	${APMDST}/apm_init.inc

APMDEPS= \
	${APMDIR}/Makefile \
	${APMDIR}/apm_bios.h \
	${APMDIR}/apm_init.S \
	${APMDIR}/apm_segments.h \
	${APMDIR}/bin2asm.sh \
	${APMDIR}/real_prot.S \
	${APMDIR}/real_prot.h \
	${APMDIR}/rmaouthdr \
	${APMDIR}/table.c

${APMINC}:	${APMDEPS} ${APMDST}
	@echo making sure the apm grappling hook is up to date...
	@(cd ${APMDST} && ${MAKE} -f ${APMDIR}/Makefile \
	  APMCC="${CC}" \
	  APMCFLAGS="${CFLAGS}" \
	  APMLD="${LD}" \
	  APMSTRIP="${APMSTRIP}" \
	  APMDIR="${APMDIR}" apm_init.inc)

clean:: 		.NOTMAIN __always_make_apmlib
	@echo cleaning the apm grappling hook objects
	@(cd ${APMDST} && ${MAKE} -f ${APMDIR}/Makefile \
	  APMCC="${CC}" \
	  APMCFLAGS="${CFLAGS}" \
	  APMLD="${LD}" \
	  APMSTRIP="${APMSTRIP}" \
	  APMDIR="${APMDIR}" clean)

${APMDST} __always_make_apmlib:	.NOTMAIN
	@([ -d ${APMDST} ] || mkdir -p ${APMDST})
