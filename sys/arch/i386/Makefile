#	$NetBSD: Makefile,v 1.18.2.1 2001/04/09 01:53:25 nathanw Exp $

# Makefile for i386 tags file and boot blocks

TI386=	${SYSDIR}/arch/i386/tags
SI386=	${SYSDIR}/arch/i386/i386/*.[ch] ${SYSDIR}/arch/i386/include/*.h \
	${SYSDIR}/arch/i386/eisa/*.[ch] ${SYSDIR}/arch/i386/isa/*.[ch] \
	${SYSDIR}/arch/i386/pci/*.[ch]
AI386=	${SYSDIR}/arch/i386/i386/*.s ${SYSDIR}/arch/i386/isa/*.s

# Directories in which to place tags links
DI386=	i386 eisa isa include pci

.include "../../kern/Make.tags.inc"

tags:
	rm -f ${TI386}
	-echo ${SI386} ${COMM} | xargs ctags -wadtf ${TI386}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AI386} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
		>> ${TI386}
	sort -o ${TI386} ${TI386}

links:
	-for i in ${DI386}; do \
	    (cd $$i && rm -f tags; ln -s ../tags tags); done


SUBDIR=	include stand

.include <bsd.subdir.mk>
