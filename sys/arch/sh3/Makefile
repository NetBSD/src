#	$NetBSD: Makefile,v 1.1 1999/09/13 10:31:06 itojun Exp $

# Makefile for sh3 tags file and boot blocks

TSH3=	../sh3/tags
SSH3=	../sh3/sh3/*.[ch] ../sh3/include/*.h \
	../sh3/eisa/*.[ch] ../sh3/isa/*.[ch] ../sh3/pci/*.[ch]
ASH3=	../sh3/sh3/*.s

# Directories in which to place tags links
DSH3=	include sh3

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TSH3} ${SSH3} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AI386} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
		>> ${TSH3}
	sort -o ${TSH3} ${TSH3}

links:
	-for i in ${DSH3}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done

etags:
	emacs-etags ${SSH3} ${COMM}

elinks:
	for i in ${DSH3}; do \
	    cd $$i && rm -f TAGS; ln -s ../TAGS TAGS; cd ..; done

SUBDIR= include

.include <bsd.subdir.mk>
