# $NetBSD: Makefile,v 1.7.6.2 1997/05/07 17:28:48 mycroft Exp $

# Makefile for alpha tags file and boot blocks

NOPROG=	noprog
NOMAN=	noman

SUBDIR=	stand

TALPHA=	../alpha/tags
SA0=	../alpha/include/pmap.old.h ../alpha/alpha/pmap.old.c
SA1=	../alpha/alpha/*.[ch]
SALPHA=	${SA0} ${SA1} ../alpha/include/*.h ../alpha/pci/*.[ch] \
	../alpha/tlsb/*.[ch] ../alpha/tc/*.[ch]
AALPHA=	../alpha/alpha/*.S

# Directories in which to place alpha tags links
DALPHA=	tc include
.include "../../kern/Make.tags.inc"

tags:	FRC
	-ctags -wdtf ${TALPHA} ${SALPHA} ${COMM} 
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AALPHA} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
		>> ${TALPHA}
	sort -o ${TALPHA} ${TALPHA}

links:
	-for i in ${DALPHA}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done

.include <bsd.prog.mk>

FRC:
