#	$NetBSD: Makefile,v 1.1 2002/03/07 14:43:56 simonb Exp $

# Makefile for evbmips tags file and boot blocks

# Find where mips source files are for inclusion in tags
.include <../mips/Makefile.inc>

# XXX !  fix this
TEVBMIPS= ../evbmips/tags
SEVBMIPS= ../evbmips/evbmips/*.[ch] ../evbmips/include/*.h ../evbmips/isa/*.[ch] \
	../evbmips/pci/*.[ch]
AEVBMIPS= ../evbmips/evbmips/*.S

# Directories in which to place tags links
DEVBMIPS= evbmips include isa pci

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TEVBMIPS} ${SEVBMIPS} ${SMIPS} ${COMM}
	egrep "^LEAF(.*)|^[AN]LEAF(.*)|^NON_LEAF(.*)" ${AEVBMIPS} ${AMIPS} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TARC}
	sort -o ${TEVBMIPS} ${TEVBMIPS}

links:
	-for i in ${DEVBMIPS}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	include

.include <bsd.subdir.mk>
