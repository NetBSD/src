#	$NetBSD: Makefile,v 1.9 2009/08/07 18:48:38 snj Exp $

# Makefile for macppc tags file

TIMACPPC=	${SYSDIR}/arch/macppc/tags
SIMACPPC=	${SYSDIR}/arch/macppc/macppc/*.[ch] \
		${SYSDIR}/arch/macppc/include/*.h \
		${SYSDIR}/arch/macppc/dev/*.[ch] \
		${SYSDIR}/arch/macppc/pci/*.[ch] \
		${SYSDIR}/external/isc/atheros_hal/dist/*.[ch] \
		${SYSDIR}/external/isc/atheros_hal/dist/*/*.[ch] \
		${SYSDIR}/external/isc/atheros_hal/ic/*.[ch]
SIMACPPC+=	${SYSDIR}/arch/powerpc/powerpc/*.[ch] \
		${SYSDIR}/arch/powerpc/include/*.h \
		${SYSDIR}/arch/powerpc/pci/*.[ch]
AIMACPPC=	${SYSDIR}/arch/macppc/macppc/*.[sS]

# Directories in which to place tags links
DIMACPPC=	macppc dev include pci

.include "../../kern/Make.tags.inc"

tags:
	-rm -f ${TIMACPPC}
	-echo ${SIMACPPC} | xargs ctags -wadtf ${TIMACPPC}
	-${FINDCOMM} | xargs ctags -wadtf ${TIMACPPC}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AIMACPPC} | \
	    ${TOOL_SED} -e \
	"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3	\1	/^\2(\3\4$$/;" \
		>> ${TIMACPPC}
	sort -o ${TIMACPPC} ${TIMACPPC}

links:
	-for i in ${DIMACPPC}; do \
	    (cd $$i && rm -f tags; ln -s ../tags tags); done

SUBDIR=	compile include stand ../powerpc/include

.include <bsd.subdir.mk>
