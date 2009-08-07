#	$NetBSD: Makefile,v 1.35.4.1 2009/08/07 18:28:20 snj Exp $

# Makefile for i386 tags file and boot blocks

TI386=	${SYSDIR}/arch/i386/tags
SI386=	${SYSDIR}/arch/i386/acpi/*.[ch] \
	${SYSDIR}/arch/i386/eisa/*.[ch] \
	${SYSDIR}/arch/i386/i386/*.[ch] \
	${SYSDIR}/arch/i386/include/*.h \
	${SYSDIR}/arch/i386/isa/*.[ch] \
	${SYSDIR}/arch/i386/mca/*.[ch] \
	${SYSDIR}/arch/i386/pci/*.[ch] \
	${SYSDIR}/arch/i386/pnpbios/*.[ch] \
	${SYSDIR}/external/isc/atheros_hal/dist/*.[ch] \
	${SYSDIR}/external/isc/atheros_hal/dist/*/*.[ch] \
	${SYSDIR}/external/isc/atheros_hal/ic/*.[ch]
SI386+=	${SYSDIR}/arch/x86/x86/*.[ch] \
	${SYSDIR}/arch/x86/acpi/*.[ch] \
	${SYSDIR}/arch/x86/include/*.h \
	${SYSDIR}/arch/x86/isa/*.[ch] \
	${SYSDIR}/arch/x86/pci/*.[ch]
AI386=	${SYSDIR}/arch/i386/i386/*.[sS]

# Directories in which to place tags links
DI386=	i386 eisa isa include pci

.include "../../kern/Make.tags.inc"

tags:
	-rm -f ${TI386}
	-echo ${SI386} | xargs ctags -wadtf ${TI386}
	-${FINDCOMM} | xargs ctags -wadtf ${TI386}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AI386} | \
	    ${TOOL_SED} -e \
	"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3	\1	/^\2(\3\4$$/;" \
		>> ${TI386}
	sort -o ${TI386} ${TI386}

links:
	-for i in ${DI386}; do \
	    (cd $$i && rm -f tags; ln -s ../tags tags); done


SUBDIR=	compile include stand ../x86/include ../xen/include

.include <bsd.subdir.mk>
