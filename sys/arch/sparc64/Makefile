#	$NetBSD: Makefile,v 1.7.12.1 2000/08/07 00:38:25 mrg Exp $

# Makefile for sparc64 tags file and boot blocks

TSPARC64=	../sparc64/tags
SSPARC64=	../sparc64/dev/*.[ch] ../../sparc/sparc/fpu/*.[ch] \
	../sparc64/include/*.[ch] ../sparc64/sparc64/*.[ch]
ASPARC64=	../sparc64/sparc64/*.s
# CSPARC64=	../../compat/*/*.[ch]

# Directories in which to place tags links
DSPARC64=	conf dev fpu include sparc64

SYSDIR=		../..
.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TSPARC64} ${SSPARC64} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${ASPARC64} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> tags
	sort -o ${TSPARC64} ${TSPARC64}

.ifmake TAGS
COMM!=  find ${SYSDIR} -name "arch" -prune -o -name "*.[ch]" -print 
.endif

TAGS: ${SSPARC64} ${COMM} ${CSPARC64} ${ASPARC64}
	etags  ${SSPARC64} ${COMM} ${CSPARC64} "--regex=/ENTRY(.*)/" \
		"--regex=/FUNC(.*)/" "--regex=/SYSCALL(.*)/" ${ASPARC64}

links:
	-for i in ${DSPARC64}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR= include stand

.include <bsd.subdir.mk>
