#	$NetBSD: Makefile,v 1.6 1997/10/11 09:10:31 mycroft Exp $

# Makefile for sparc tags file and boot blocks

TSPARC=	../sparc/tags
SSPARC=	../sparc/dev/*.[ch] ../sparc/fpu/*.[ch] \
	../sparc/include/*.[ch] ../sparc/sparc/*.[ch]
ASPARC=	../sparc/sparc/*.s

# Directories in which to place tags links
DSPARC=	conf dev fpu include sparc

tags:
	-ctags -wdtf ${TSPARC} ${SSPARC} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${ASPARC} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> tags
	sort -o ${TSPARC} ${TSPARC}

links:
	-for i in ${DSPARC}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR= stand

.include <bsd.subdir.mk>
