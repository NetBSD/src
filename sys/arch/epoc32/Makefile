#	$NetBSD: Makefile,v 1.1 2013/05/01 13:11:59 kiyohara Exp $

# Makefile for epoc32 tags file and boot blocks

TEPOC32=	../epoc32/tags
SEPOC32=	../epoc32/epoc32/*.[ch] ../epoc32/include/*.h \
		../epoc32/dev/*.[ch]
AEPOC32=	../epoc32/epoc32/*.S

# Directories in which to place tags links
DEPOC32=	include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TEPOC32} ${SEPOC32} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AEPOC32} | \
	    ${TOOL_SED} -e \
		"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TEPOC32}
	sort -o ${TEPOC32} ${TEPOC32}

links:
	-for i in ${DEPOC32}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done

SUBDIR=	compile include stand

.include <bsd.subdir.mk>
