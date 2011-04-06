#	$NetBSD: Makefile,v 1.3 2011/04/06 01:31:20 dyoung Exp $

# Makefile for landisk tags file and boot blocks

# Find where SH3 source files are for inclusion in tags
.include <../sh3/Makefile.inc>

TLANDISK=	${SYSDIR}/arch/landisk/tags
SLANDISK=	${SYSDIR}/arch/landisk/landisk/*.[ch]
SLANDISK+=	${SYSDIR}/arch/landisk/include/*.h
SLANDISK+=	${SYSDIR}/arch/landisk/dev/*.[ch]

ALANDISK=	${SYSDIR}/arch/landisk/landisk/*.S

# Directories in which to place tags links
DLANDISK=	landisk include

.include "../../kern/Make.tags.inc"

tags:
	-rm -f ${TLANDISK}
	-echo ${SLANDISK} ${SSH3} | xargs ctags -wadtf ${TLANDISK}
	-${FINDCOMM} | xargs ctags -wadtf ${TLANDISK}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${ALANDISK} ${ASH3} | \
	    ${TOOL_SED} -e \
		"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3	\1	/^\2(\3\4$$/;" \
	    >> ${TLANDISK}
	sort -o ${TLANDISK} ${TLANDISK}

links:
	-for i in ${DLANDISK}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	compile include stand

.include <bsd.subdir.mk>
