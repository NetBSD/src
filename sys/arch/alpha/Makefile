#	$Id: Makefile,v 1.1 1995/02/13 23:06:39 cgd Exp $

# Makefile for alpha links

.include "../kern/Make.tags.inc"

all:
	@echo "make links or tags only"

DIRS=	alpha conf dev include osf

links::
	-for i in ${DIRS}; do \
	    (cd $$i && { rm -f tags; ln -s ${SYSTAGS} tags; }) done

# XXX should be absolute paths
ALPHA=	alpha/*.[ch] conf/*.[ch] dev/*.[ch] include/*.[ch] osf/*.[ch]
AALPHA=	alpha/*.s

tags::
	-ctags -wdt ${COMM} ${PMAX}
	egrep "^LEAF(.*)|^[AN]LEAF(.*)|^NON_LEAF(.*)" ${APMAX} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
		>> tags
	sort -o tags tags
	chown bin.wsrc tags
	chmod 444 tags
