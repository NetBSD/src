#	$NetBSD: Makefile,v 1.3.38.1 2014/08/20 00:03:23 tls Exp $

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
	    ${TOOL_SED} -e \
		"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TSHARK}
	sort -o ${TSHARK} ${TSHARK}

links:
	-for i in ${DSHARK}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	compile include
#SUBDIR+=	stand

.include <bsd.subdir.mk>
