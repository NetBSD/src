#	$NetBSD: Makefile,v 1.1.2.3 2003/01/07 21:14:36 thorpej Exp $

# Makefile for shark tags file and boot blocks

TSHARK=	../shark/tags
SSHARK=	../shark/shark/*.[ch] ../shark/include/*.h \
	../shark/isa/*.[ch] ../shark/ofw/*.[ch]
ASHARK=	../shark/isa/*.S ../shark/shark/*.S

# Directories in which to place tags links
DSHARK=	isa include ofw

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TSHARK} ${SSHARK} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${ASHARK} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TSHARK}
	sort -o ${TSHARK} ${TSHARK}

links:
	-for i in ${DSHARK}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	compile include # stand

.include <bsd.subdir.mk>
