#	$NetBSD: Makefile,v 1.2.136.1 2011/06/06 09:06:33 jruoho Exp $

.include <../powerpc/Makefile.inc>

TSANDPOINT=	${SYSDIR}/arch/sandpoint/tags
SSANDPOINT=	${SYSDIR}/arch/sandpoint/sandpoint/*.[ch] \
		${SYSDIR}/arch/sandpoint/isa/*.[ch] \
		${SYSDIR}/arch/sandpoint/pci/*.[ch] \
		${SYSDIR}/arch/sandpoint/include/*.h
ASANDPOINT=	${SYSDIR}/arch/sandpoint/sandpoint/*.S

DSANDPOINT=	include

.include "../../kern/Make.tags.inc"

tags:
	-rm -f ${TSANDPOINT}
	-echo ${SSANDPOINT} ${SPPC} | xargs ctags -wadtf ${TSANDPOINT}
	-${FINDCOMM} | xargs ctags -wadtf ${TSANDPOINT}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${ASANDPOINT} ${APPC} | \
	    ${TOOL_SED} -e \
		"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3	\1	/^\2(\3\4$$/;" \
	    >> ${TSANDPOINT}
	sort -o ${TSANDPOINT} ${TSANDPOINT}

links:
	-for i in ${DSANDPOINT}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	compile include stand

.include <bsd.subdir.mk>
