#	$NetBSD: Makefile,v 1.1.6.3 2000/12/08 09:26:22 bouyer Exp $

# Makefile for arm26 tags file and boot blocks

TARM26=	../arm26/tags
SARM26=	../arm26/arm26/*.[ch] ../arm26/include/*.h \
	../arm26/iobus/*.[ch] ../arm26/ioc/*.[ch] ../arm32/mainbus/*.[ch] \
	../arm26/vidc/*.[ch]
AARM26=	../arm26/arm26/*.S ../arm26/iobus/*.S

# Directories in which to place tags links
DARM26=	arm26 include iobus ioc mainbus vidc

.include "../../kern/Make.tags.inc"

tags: TAGS
	-ctags -wdtf ${TARM26} ${SARM26} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AARM26} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TARM26}
	sort -o ${TARM26} ${TARM26}

TAGS:
	etags ${SARM26} ${COMM} ${AARM26}

links:
	-for i in ${DARM26}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	stand include

.include <bsd.subdir.mk>
