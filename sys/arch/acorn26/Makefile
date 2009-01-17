#	$NetBSD: Makefile,v 1.2.104.1 2009/01/17 13:27:46 mjf Exp $

# Makefile for acorn26 tags file and boot blocks

TACORN26=	../acorn26/tags
SACORN26=	../acorn26/acorn26/*.[ch] ../acorn26/include/*.h \
	../acorn26/iobus/*.[ch] ../acorn26/ioc/*.[ch] ../arm32/mainbus/*.[ch] \
	../acorn26/vidc/*.[ch]
AACORN26=	../acorn26/acorn26/*.S ../acorn26/iobus/*.S

# Directories in which to place tags links
DACORN26=	acorn26 include iobus ioc mainbus vidc

.include "../../kern/Make.tags.inc"

tags: TAGS
	-ctags -wdtf ${TACORN26} ${SACORN26} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AACORN26} | \
	    ${TOOL_SED} -e \
		"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TACORN26}
	sort -o ${TACORN26} ${TACORN26}

TAGS:
	etags ${SACORN26} ${COMM} ${AACORN26}

links:
	-for i in ${DACORN26}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	compile include stand

.include <bsd.subdir.mk>
