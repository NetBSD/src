#	$NetBSD: Makefile,v 1.3 2000/06/28 09:47:55 soren Exp $

# Makefile for cobalt tags file

# Find where mips source files are for inclusion in tags
.include <../mips/Makefile.inc>

TCOBALT=	../cobalt/tags
SCOBALT=	../cobalt/cobalt/*.[ch] ../cobalt/include/*.h \
		../cobalt/dev/*.[ch] ../cobalt/pci/*.[ch]
ACOBALT=	../cobalt/cobalt/*.S

# Directories in which to place tags links
DCOBALT=	pci include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TCOBALT} ${SCOBALT} ${SMIPS} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${ACOBALT} ${AMIPS} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
		>> ${TCOBALT}
	sort -o ${TCOBALT} ${TCOBALT}

links:
	-for i in ${DCOBALT}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done

SUBDIR=	include stand

.include <bsd.subdir.mk>
