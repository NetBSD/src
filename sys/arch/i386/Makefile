#	$NetBSD: Makefile,v 1.30.8.1 2008/01/10 23:43:17 bouyer Exp $

# Makefile for i386 tags file and boot blocks

TI386=	${SYSDIR}/arch/i386/tags
SI386=	${SYSDIR}/arch/i386/acpi/*.[ch] \
	${SYSDIR}/arch/i386/eisa/*.[ch] \
	${SYSDIR}/arch/i386/i386/*.[ch] \
	${SYSDIR}/arch/i386/include/*.h \
	${SYSDIR}/arch/i386/isa/*.[ch] \
	${SYSDIR}/arch/i386/pci/*.[ch] \
	${SYSDIR}/arch/i386/pnpbios/*.[ch] \
	${SYSDIR}/contrib/dev/ath/*.[ch] \
	${SYSDIR}/contrib/dev/ath/netbsd/*.[ch] \
	${SYSDIR}/contrib/dev/ath/public/*.[ch]
SI386+=	${SYSDIR}/arch/x86/x86/*.[ch] ${SYSDIR}/arch/x86/include/*.h \
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
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3	\1	/^\2(\3\4$$/;" \
		>> ${TI386}
	sort -o ${TI386} ${TI386}

links:
	-for i in ${DI386}; do \
	    (cd $$i && rm -f tags; ln -s ../tags tags); done


SUBDIR=	compile include stand ../x86/include

.include <bsd.subdir.mk>
