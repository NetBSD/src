#	$NetBSD: Makefile,v 1.12 1997/10/11 08:44:46 mycroft Exp $

# Makefile for i386 tags file and boot blocks

SUBDIR=	stand

TI386=	../i386/tags
SI386=	../i386/i386/*.[ch] ../i386/include/*.h \
	../i386/eisa/*.[ch] ../i386/isa/*.[ch] ../i386/pci/*.[ch]
AI386=	../i386/i386/*.s

# Directories in which to place i386 tags links
DI386=	eisa isa include pci

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TI386} ${SI386} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AI386} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
		>> ${TI386}
	sort -o ${TI386} ${TI386}

links:
	-for i in ${DI386}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done

.include <bsd.subdir.mk>
