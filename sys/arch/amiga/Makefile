#	from: @(#)Makefile	7.3 (Berkeley) 6/9/91
#	$Id: Makefile,v 1.3 1993/10/30 23:40:05 mw Exp $

COMM=	../net/*.[ch] ../netimp/*.[ch] ../netinet/*.[ch] ../netns/*.[ch] \
	../netiso/*.[ch] ../netccitt/*.[ch] \
	../kern/*.c ../ufs/*.[ch] ../nfs/*.[ch] ../vm/*.[ch] ../sys/*.h

# Makefile for amiga tags file

all:
	@echo "make tags or links only"

TAMIGA=	../amiga/tags
SAMIGA=	../amiga/amiga/*.[ch] ../amiga/include/*.h \
	../amiga/dev/*.[ch]
AAMIGA=	../amiga/amiga/*.s

# Directories in which to place amiga tags links
DAMIGA=	dev include

tags:
	-ctags -dtf ${TAMIGA} ${COMM} ${SAMIGA}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AAMIGA} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
		>> ${TAMIGA}
	sort -o ${TAMIGA} ${TAMIGA}

links:
	-for i in ${DAMIGA}; do \
	    cd ../$$i && rm -f tags; ln -s ../tags tags; done
