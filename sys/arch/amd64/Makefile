#	$NetBSD: Makefile,v 1.5.8.1 2010/10/24 22:47:51 jym Exp $

# Makefile for amd64 tags file and boot blocks

TAMD64=		${SYSDIR}/arch/amd64/tags
SAMD64=		${SYSDIR}/arch/amd64/amd64/*.[ch] \
		${SYSDIR}/arch/amd64/include/*.h
AAMD64=		${SYSDIR}/arch/amd64/amd64/*.S

# Directories in which to place tags links
DAMD64=		amd64 isa include pci

.include "../../kern/Make.tags.inc"

tags:
	-rm -f ${TAMD64}
	-echo ${SAMD64} | xargs ctags -wadtf ${TAMD64}
	-find -H ${SYSDIR}/external/intel-public/acpica/dist/ -name '*.[ch]' | \
	    sort -t / -u | xargs ctags -wadtf ${TAMD64}
	-${FINDCOMM} | xargs ctags -wadtf ${TAMD64}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AAMD64} | \
	    ${TOOL_SED} -e \
		"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
		>> ${TAMD64}
	sort -o ${TAMD64} ${TAMD64}

links:
	-for i in ${DAMD64}; do \
	    (cd $$i && rm -f tags; ln -s ../tags tags); done


SUBDIR=	compile include ../x86/include ../xen/include \
	../i386/include ../i386/stand

.include <bsd.subdir.mk>
