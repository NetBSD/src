#	$NetBSD: Makefile,v 1.26.6.1 2006/04/22 11:37:31 simonb Exp $

# Makefile for i386 tags file and boot blocks

TI386=	${SYSDIR}/arch/i386/tags
SI386=	${SYSDIR}/arch/i386/i386/*.[ch] ${SYSDIR}/arch/i386/include/*.h \
	${SYSDIR}/arch/i386/eisa/*.[ch] ${SYSDIR}/arch/i386/isa/*.[ch] \
	${SYSDIR}/arch/i386/pci/*.[ch] \
	${SYSDIR}/contrib/arch/i386/dev/*.[ch] \
	${SYSDIR}/contrib/dev/ic/*.[ch]
SI386+=	${SYSDIR}/arch/x86/x86/*.[ch] ${SYSDIR}/arch/x86/include/*.h \
	${SYSDIR}/arch/x86/isa/*.[ch] \
	${SYSDIR}/arch/x86/pci/*.[ch]
AI386=	${SYSDIR}/arch/i386/i386/*.[sS] ${SYSDIR}/arch/i386/isa/*.[sS]

# Directories in which to place tags links
DI386=	i386 eisa isa include pci

.include "../../kern/Make.tags.inc"

tags:
	rm -f ${TI386}
	-echo ${SI386} ${COMM} | xargs ctags -wadtf ${TI386}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AI386} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3	\1	/^\2(\3\4$$/;" \
		>> ${TI386}
	sort -o ${TI386} ${TI386}

links:
	-for i in ${DI386}; do \
	    (cd $$i && rm -f tags; ln -s ../tags tags); done


SUBDIR=	compile include stand ../x86/include

.include <bsd.subdir.mk>
