#	$NetBSD: Makefile,v 1.6 2003/03/06 22:22:55 fvdl Exp $

# Makefile for x86_64 tags file and boot blocks

TX86_64=	${SYSDIR}/arch/x86_64/tags
SX86_64=	${SYSDIR}/arch/x86_64/x86_64/*.[ch] \
		${SYSDIR}/arch/x86_64/include/*.h \
		${SYSDIR}/arch/x86_64/isa/*.[ch] \
		${SYSDIR}/arch/x86_64/pci/*.[ch]
AX86_64=	${SYSDIR}/arch/x86_64/x86_64/*.s ${SYSDIR}/arch/x86_64/isa/*.s

# Directories in which to place tags links
DX86_64=	x86_64 isa include pci

.include "../../kern/Make.tags.inc"

tags:
	rm -f ${TX86_64}
	-echo ${SX86_64} ${COMM} | xargs ctags -wadtf ${TX86_64}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AX86_64} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
		>> ${TX86_64}
	sort -o ${TX86_64} ${TX86_64}

links:
	-for i in ${DX86_64}; do \
	    (cd $$i && rm -f tags; ln -s ../tags tags); done


SUBDIR=	compile include ../x86/include ../i386/stand

.include <bsd.subdir.mk>
