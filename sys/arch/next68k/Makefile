#	$NetBSD: Makefile,v 1.5 2001/04/03 17:12:23 jhawk Exp $

# Makefile for next68k tags file and boot blocks

# Find where m68k source files are for inclusion in tags
.include <../m68k/Makefile.inc>

ENEXT68K=	../next68k/TAGS
TNEXT68K=	../next68k/tags
SNEXT68K=	../next68k/next68k/*.[ch] ../next68k/include/*.h \
	../next68k/dev/*.[ch]
ANEXT68K=	../next68k/next68k/*.s

# Directories in which to place tags links
DNEXT68K=	dev next68k include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TNEXT68K} ${SNEXT68K} ${SM68K} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${ANEXT68K} ${AM68K} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TNEXT68K}
	sort -o ${TNEXT68K} ${TNEXT68K}

.ifmake TAGS
COMM!=  find ${SYSDIR} -name "arch" -prune -o -name "*.[ch]" -print 
.endif

TAGS:
	emacs-etags -o ${ENEXT68K} ${SNEXT68K} ${SM68K} ${COMM}
	emacs-etags -a -l none \
		--regex='/^\(ALT\)?ENTRY\(_NOPROFILE\)?(\([a-zA-Z0-9_]*\))\|^GLOBAL(\([a-zA-Z0-9_]*\))/\3\4/' \
		-o ${ENEXT68K} ${ANEXT68K} ${AM68K}

links:
	-for i in ${DNEXT68K}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR= include stand

.include <bsd.subdir.mk>
