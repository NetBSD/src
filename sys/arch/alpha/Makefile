# $NetBSD: Makefile,v 1.8 1997/10/11 08:42:02 mycroft Exp $

NOOBJ=

# Makefile for alpha tags file and boot blocks

SUBDIR=	stand

TALPHA=	../alpha/tags
SALPHA=	../alpha/alpha/*.[ch] ../alpha/include/*.h \
	../alpha/pci/*.[ch] ../alpha/tlsb/*.[ch] ../alpha/tc/*.[ch]
AALPHA=	../alpha/alpha/*.S

# Directories in which to place alpha tags links
DALPHA=	tc include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TALPHA} ${SALPHA} ${COMM} 
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AALPHA} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TALPHA}
	sort -o ${TALPHA} ${TALPHA}

links:
	-for i in ${DALPHA}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done

.include <bsd.subdir.mk>
