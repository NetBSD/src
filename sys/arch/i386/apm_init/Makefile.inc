#	$NetBSD: Makefile.inc,v 1.2 1996/08/30 02:32:18 jtk Exp $
#
#	NOTE: $S must correspond to the top of the 'sys' tree

APMDIR=	${I386}/apm_init

APMDST=	lib/apm_init
APMINC?=	${APMDST}/apm_init.inc

APMDEPS= \
	${APMDIR}/Makefile \
	${APMDIR}/apm_init.S \
	${APMDIR}/bin2asm.sh \
	${APMDIR}/real_prot.S \
	${APMDIR}/real_prot.h \
	${APMDIR}/rmaouthdr \
	${APMDIR}/table.c \
	${I386}/include/apmvar.h

${APMINC}:	${APMDEPS} ${APMDST}
	@echo making sure the apm grappling hook is up to date...
	@(cd ${APMDST} && ${MAKE} -f ${APMDIR}/Makefile \
	  APMCC="${CC}" \
	  APMCFLAGS="${CFLAGS}" \
	  APMLD="${LD}" \
	  APMSTRIP="${APMSTRIP}" \
	  APMDIR="${APMDIR}" I386="${I386}" S="${S}" \
	  apm_init.inc)

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
