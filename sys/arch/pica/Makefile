#	$NetBSD: Makefile,v 1.2 1996/07/16 23:23:55 thorpej Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/16/93

# Makefile for pica links, tags file

.include "../kern/Make.tags.inc"

all:
	@echo "make links or tags only"

DIRS=	conf dev dist include pica

links::
	-for i in ${DIRS}; do \
	    (cd $$i && { rm -f tags; ln -s ${SYSTAGS} tags; }) done

PICA=	/sys/pica/dev/*.[ch] /sys/pica/include/*.[ch] \
	/sys/pica/pica/*.[ch] /sys/pica/ultrix/*.[ch]
APICA=	/sys/pica/pica/*.s

tags::
	-ctags -wdt ${COMM} ${PICA}
	egrep "^LEAF(.*)|^[AN]LEAF(.*)|^NON_LEAF(.*)" ${APICA} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
		>> tags
	sort -o tags tags
	chown bin.wsrc tags
	chmod 444 tags
