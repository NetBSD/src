#	$NetBSD: Makefile,v 1.4 2011/04/06 01:43:13 dyoung Exp $

# Makefile for sbmips tags file and boot blocks

# Find where mips source files are for inclusion in tags
.include <../mips/Makefile.inc>

# XXX !  fix this
TSBMIPS=	${SYSDIR}/arch/sbmips/tags
SSBMIPS=	${SYSDIR}/arch/sbmips/sbmips/*.[ch]
SSBMIPS+=	${SYSDIR}/arch/sbmips/include/*.h
ASBMIPS=	${SYSDIR}/arch/sbmips/sbmips/*.S

# Directories in which to place tags links
DSBMIPS= sbmips include

.include "../../kern/Make.tags.inc"

tags:
	-rm -f ${TSBMIPS}
	-echo ${SSBMIPS} ${SMIPS} | xargs ctags -wadtf ${TSBMIPS}
	-${FINDCOMM} | xargs ctags -wadtf ${TSBMIPS}
	egrep "^LEAF(.*)|^[AN]LEAF(.*)|^NON_LEAF(.*)" ${ASBMIPS} ${AMIPS} | \
	    ${TOOL_SED} -e \
		"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3	\1	/^\2(\3\4$$/;" \
	    >> ${TSBMIPS}
	sort -o ${TSBMIPS} ${TSBMIPS}

links:
	-for i in ${DSBMIPS}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	compile include stand

.include <bsd.subdir.mk>
