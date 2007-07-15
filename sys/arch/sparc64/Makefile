#	$NetBSD: Makefile,v 1.13.60.1 2007/07/15 13:17:01 ad Exp $

# Makefile for sparc64 tags file and boot blocks

TSPARC64=	${SYSDIR}/arch/sparc64/tags
SSPARC64=	${SYSDIR}/arch/sparc64/dev/*.[ch] \
		${SYSDIR}/arch/sparc/fpu/*.[ch] \
		${SYSDIR}/arch/sparc64/include/*.[ch] \
		${SYSDIR}/arch/sparc64/sparc64/*.[ch]
ASPARC64=	${SYSDIR}/arch/sparc64/sparc64/*.[sS]
# CSPARC64=	../../compat/*/*.[ch]

# Directories in which to place tags links
DSPARC64=	conf dev fpu include sparc64

.include "../../kern/Make.tags.inc"

tags:
	-rm ${TSPARC64}
	-echo ${SSPARC64} ${COMM} | xargs ctags -wadtf ${TSPARC64}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${ASPARC64} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> tags
	sort -o ${TSPARC64} ${TSPARC64}

links:
	-for i in ${DSPARC64}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done

SUBDIR= compile include ../sparc/include stand

.include <bsd.subdir.mk>
