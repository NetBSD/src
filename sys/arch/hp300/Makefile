#	$NetBSD: Makefile,v 1.6 1997/05/07 17:28:55 mycroft Exp $

#	@(#)Makefile	7.3 (Berkeley) 6/9/91

# Makefile for hp300 tags file and boot blocks

NOPROG=	noprog
NOMAN=	noman

SUBDIR= stand

THP300=	../hp300/tags
SHP300=	../hp300/hp300/*.[ch] ../hp300/include/*.h \
	../hp300/dev/*.[ch]
AHP300=	../hp300/hp300/*.s

# Directories in which to place hp300 tags links
DHP300=	dev hp300 include

tags:
	-ctags -dtf ${THP300} ${COMM} ${SHP300}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AHP300} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
		>> ${THP300}
	sort -o ${THP300} ${THP300}

links:
	-for i in ${DHP300}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done

.include <bsd.prog.mk>
