#	$NetBSD: Makefile,v 1.18 2009/05/19 22:01:34 dyoung Exp $

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
	-rm -f ${TSPARC64}
	-echo ${SSPARC64} | xargs ctags -wadtf ${TSPARC64}
	${FINDCOMM} | xargs ctags -wadtf ${TSPARC64}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${ASPARC64} | \
	    ${TOOL_SED} -e "s;/;\\/;g" | \
	    ${TOOL_SED} -e \
		"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3	\1	/^\2(\3\4$$/;" \
	    >> tags
	sort -o ${TSPARC64} ${TSPARC64}

links:
	-for i in ${DSPARC64}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done

SUBDIR= compile include ../sparc/include stand

.include <bsd.subdir.mk>
