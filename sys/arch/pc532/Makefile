#	$NetBSD: Makefile,v 1.5 1997/10/11 08:44:47 mycroft Exp $

# Makefile for pc532 tags file

SUBDIR= stand

TPC532=	../pc532/tags
SPC532=	../pc532/pc532/*.[ch] ../pc532/include/*.h \
	../pc532/dev/*.[ch] ../pc532/scsi/*.[ch]
APC532=	../pc532/pc532/*.s

# Directories in which to place pc532 tags links
DPC532=	dev include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TPC532} ${SPC532} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${APC532} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
		>> ${TPC532}
	sort -o ${TPC532} ${TPC532}

links:
	-for i in ${DPC532}; do \
	    cd ../$$i && rm -f tags; ln -s ../tags tags; done

.include <bsd.subdir.mk>
