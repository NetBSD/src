#	$NetBSD: Makefile.inc,v 1.5 1996/09/09 11:34:01 jtk Exp $
#
#	NOTE: $S must correspond to the top of the 'sys' tree

APMDIR=	${I386}/apm_init

APMDST=	lib/apm_init
APMINC?=	${APMDST}/biostramp.inc

APMDEPS= \
	${APMDIR}/Makefile \
	${APMDIR}/bin2asm.sh \
	${APMDIR}/biostramp.S \
	${APMDIR}/rmaouthdr \
	${I386}/include/apmvar.h \
	${I386}/include/param.h \
	${I386}/include/segments.h \
	${I386}/include/specialreg.h \
	${I386}/include/psl.h

${APMINC}:	${APMDEPS} ${APMDST} assym.h apm.h
	@echo making sure the apm grappling hook is up to date...
	@(cd ${APMDST} && ${MAKE} -f ${APMDIR}/Makefile \
	  APMCC="${CC}" \
	  APMCFLAGS="${CFLAGS} -I${.CURDIR}" \
	  APMLD="${LD}" \
	  APMSTRIP="${APMSTRIP}" \
	  APMDIR="${APMDIR}" I386="${I386}" S="${S}" \
	  APMDEPS="${APMDEPS} ${.CURDIR}/assym.h" \
	  biostramp.inc)

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
