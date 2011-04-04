#	$NetBSD: Makefile,v 1.7 2011/04/04 19:47:21 dyoung Exp $

.include <../powerpc/Makefile.inc>

TOFPPC=	${SYSDIR}/arch/ofppc/tags
SOFPPC=	${SYSDIR}/arch/ofppc/ofppc/*.[ch] \
	${SYSDIR}/arch/ofppc/isa/*.[ch] \
	${SYSDIR}/arch/ofppc/pci/*.[ch] \
	${SYSDIR}/arch/ofppc/include/*.h
AOFPPC=	${SYSDIR}/arch/ofppc/ofppc/*.S

DOFPPC=	include

.include "../../kern/Make.tags.inc"

tags:
	-rm -f ${TOFPPC}
	-echo ${SOFPPC} ${SPPC} | xargs ctags -wadtf ${TOFPPC}
	-${FINDCOMM} | xargs ctags -wadtf ${TOFPPC}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AOFPPC} ${APPC} | \
	    ${TOOL_SED} -e \
		"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3	\1	/^\2(\3\4$$/;" \
	    >> ${TOFPPC}
	sort -o ${TOFPPC} ${TOFPPC}

links:
	-for i in ${DOFPPC}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	compile include stand

.include <bsd.subdir.mk>
