#	$NetBSD: Makefile,v 1.1 2000/06/14 15:32:11 soren Exp $

# Makefile for sgimips tags file

# Find where mips source files are for inclusion in tags
.include <../mips/Makefile.inc>

TSGIMIPS=	../sgimips/tags
SSGIMIPS=	../sgimips/sgimips/*.[ch] ../sgimips/include/*.h \
		../sgimips/gio/*.[ch] ../sgimips/pbus/*.[ch]
ASGIMIPS=	../sgimips/sgimips/*.S

# Directories in which to place tags links
DSGIMIPS=	gio pbus include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TSGIMIPS} ${SSGIMIPS} ${SMIPS} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${ASGIMIPS} ${AMIPS} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
		>> ${TSGIMIPS}
	sort -o ${TSGIMIPS} ${TSGIMIPS}

links:
	-for i in ${DSGIMIPS}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done

SUBDIR=	include

.include <bsd.subdir.mk>
