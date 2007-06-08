#	$NetBSD: Makefile,v 1.14 2007/06/08 22:59:51 dyoung Exp $

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

SYSDIR?= ../..
.ifmake TAGS
COMM!=  find ${SYSDIR} -name "arch" -prune -o -name "*.[ch]" -print 
.endif

TAGS: ${SSPARC64} ${COMM} ${CSPARC64} ${ASPARC64}
	etags  ${SSPARC64} ${COMM} ${CSPARC64} "--regex=/ENTRY(.*)/" \
		"--regex=/FUNC(.*)/" "--regex=/SYSCALL(.*)/" ${ASPARC64}

links:
	-for i in ${DSPARC64}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR= compile include ../sparc/include stand

.include <bsd.subdir.mk>
