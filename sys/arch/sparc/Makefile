# from: @(#)Makefile	8.1 (Berkeley) 6/11/93
# $Id: Makefile,v 1.1 1993/10/02 10:22:00 deraadt Exp $
#
# Makefile for sparc links, tags file

.include "../kern/Make.tags.inc"

all:
	@echo "make links or tags only"

DIRS=	conf dev fpu include rcons sbus sparc sunos

links::
	-for i in ${DIRS}; do \
	    (cd $$i && { rm -f tags; ln -s ${SYSTAGS} tags; }) done

SPARC=	/sys/arch/sparc/dev/*.[ch] /sys/arch/sparc/fpu/*.[ch] \
	/sys/arch/sparc/include/*.[ch] /sys/arch/sparc/rcons/*.[ch] \
	/sys/arch/sparc/sbus/*.[ch] /sys/arch/sparc/sparc/*.[ch] \
	/sys/arch/sparc/sunos/*.[ch]
ASPARC=	/sys/arch/sparc/sparc/*.s

tags::
	-ctags -wdt ${COMM} ${SPARC}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${ASPARC} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
		>> tags
	sort -o tags tags
	chown bin.wsrc tags
	chmod 444 tags
