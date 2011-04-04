#	$NetBSD: Makefile,v 1.4 2011/04/04 19:47:49 dyoung Exp $

.include <../powerpc/Makefile.inc>

TPREP=	${SYSDIR}/arch/prep/tags
SPREP=	${SYSDIR}/arch/prep/prep/*.[ch] \
	${SYSDIR}/arch/prep/isa/*.[ch] \
	${SYSDIR}/arch/prep/pci/*.[ch] \
	${SYSDIR}/arch/prep/include/*.h
APREP=	${SYSDIR}/arch/prep/prep/*.S

DPREP=	include

.include "../../kern/Make.tags.inc"

tags:
	-rm -f ${TPREP}
	-echo ${SPREP} ${SPPC} | xargs ctags -wadtf ${TPREP}
	-${FINDCOMM} | xargs ctags -wadtf ${TPREP}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${APREP} ${APPC} | \
	    ${TOOL_SED} -e \
		"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3	\1	/^\2(\3\4$$/;" \
	    >> ${TPREP}
	sort -o ${TPREP} ${TPREP}

links:
	-for i in ${DPREP}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	compile include stand

.include <bsd.subdir.mk>
