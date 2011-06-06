#	$NetBSD: Makefile,v 1.3.22.1 2011/06/06 09:06:17 jruoho Exp $

# Makefile for netwinder tags file and boot blocks

.include <../arm/Makefile.inc>

TNETWINDER=	${SYSDIR}/arch/netwinder/tags
SNETWINDER=	${SYSDIR}/arch/netwinder/netwinder/*.[ch] \
		${SYSDIR}/arch/netwinder/include/*.h \
		${SYSDIR}/arch/netwinder/pci/*.[ch]
ANETWINDER=	${SYSDIR}/arch/netwinder/netwinder/*.S

# Directories in which to place tags links
DNETWINDER=	pci include

.include "../../kern/Make.tags.inc"

tags:
	-rm -f ${TNETWINDER}
	-echo ${SNETWINDER} ${SARM} | xargs ctags -wadtf ${TNETWINDER}
	-${FINDCOMM} | xargs ctags -wadtf ${TNETWINDER}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${ANETWINDER} ${AARM} | \
	    ${TOOL_SED} -e \
		"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3	\1	/^\2(\3\4$$/;" \
	    >> ${TNETWINDER}
	sort -o ${TNETWINDER} ${TNETWINDER}

links:
	-for i in ${DNETWINDER}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	compile include

.include <bsd.subdir.mk>
