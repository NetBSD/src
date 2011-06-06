#	$NetBSD: Makefile,v 1.9.22.1 2011/06/06 09:06:44 jruoho Exp $

# Makefile for sparc tags file and boot blocks

TSPARC=	${SYSDIR}/arch/sparc/tags
SSPARC=	${SYSDIR}/arch/sparc/dev/*.[ch] \
	${SYSDIR}/arch/sparc/fpu/*.[ch] \
	${SYSDIR}/arch/sparc/sparc/*.[ch] \
	${SYSDIR}/arch/sparc/include/*.h
ASPARC=	${SYSDIR}/arch/sparc/sparc/*.s

# Directories in which to place tags links
DSPARC=	sparc dev fpu include

.include "../../kern/Make.tags.inc"

tags:
	-rm -f ${TSPARC}
	-echo ${SSPARC} | xargs ctags -wadtf ${TSPARC}
	-${FINDCOMM} | xargs ctags -wadtf ${TSPARC}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${ASPARC} | \
	    ${TOOL_SED} -e \
	"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3	\1	/^\2(\3\4$$/;" \
		>> ${TSPARC}
	sort -o ${TSPARC} ${TSPARC}

links:
	-for i in ${DSPARC}; do \
	    (cd $$i && rm -f tags; ln -s ../tags tags); done


SUBDIR=	compile include stand

.include <bsd.subdir.mk>
