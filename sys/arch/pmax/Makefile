# $Id: Makefile,v 1.1.1.1 1993/10/12 03:22:19 deraadt Exp $
# from:	@(#)Makefile	7.3 (Berkeley) 7/24/92

# Makefile for pmax tags file

.include "../kern/Make.tags.inc"

all:
	@echo "make tags only"

PMAX=	/sys/pmax/conf/*.[ch] /sys/pmax/dev/*.[ch] /sys/pmax/include/*.[ch] \
	/sys/pmax/pmax/*.[ch] /sys/pmax/ultrix/*.[ch]
APMAX=	/sys/pmax/pmax/*.s

tags::
	-ctags -dt ${COMM} ${PMAX}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${APMAX} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
		>> tags
	sort -o tags tags
