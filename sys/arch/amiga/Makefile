#	from: @(#)Makefile	7.3 (Berkeley) 6/9/91
#	$Id: Makefile,v 1.5 1994/02/23 20:23:30 chopps Exp $
S=../..
COMM=	$S/net/*.[ch] $S/netimp/*.[ch] $S/netinet/*.[ch] $S/netns/*.[ch] \
	$S/netiso/*.[ch] $S/netccitt/*.[ch] \
	$S/kern/*.c $S/ufs/*.[ch] $S/nfs/*.[ch] $S/vm/*.[ch] $S/sys/*.h

# Makefile for amiga tags file

all:
	@echo "make tags or links only"

TAMIGA=	$S/arch/amiga/tags
SAMIGA=	$S/arch/amiga/amiga/*.[ch] $S/arch/amiga/include/*.h \
	$S/arch/amiga/dev/*.[ch]
AAMIGA=	$S/arch/amiga/amiga/*.s

# Directories in which to place amiga tags links
DAMIGA=	amiga dev include

tags:
	-ctags -dtf ${TAMIGA} ${COMM} ${SAMIGA}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AAMIGA} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
		>> ${TAMIGA}
	sort -o ${TAMIGA} ${TAMIGA}

links:
	-for i in ${DAMIGA}; do \
	    rm -f $$i/tags; ln -s ../tags $$i/tags; done
install:

