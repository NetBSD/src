#	$NetBSD: Makefile,v 1.1 2001/06/13 06:01:50 simonb Exp $

ETAGS?= etags

SUBDIR=	include

SWALNUT=	walnut/*.[ch] include/*.h dev/*.[ch] pci/*.[ch]
AWALNUT=	walnut/*.[sS]

SPPC=		../powerpc/ibm4xx/*.[ch] ../powerpc/powerpc/*.[ch] \
		../powerpc/include/*.h ../powerpc/include/ibm4xx/*.h \
		../powerpc/fpu/*.[ch]
APPC=		../powerpc/ibm4xx/*.[sS] ../powerpc/powerpc/*.[sS]
LPPC=		../../lib/libkern/arch/powerpc/*.[chsS] \
		../../lib/libkern/*.[ch] \
		../../lib/libsa/*.[ch]

SYSDIR=		../..
.include "../../kern/Make.tags.inc"

.ifmake TAGS
COMM!=  find ${SYSDIR} -name "arch" -prune -o -name "*.[ch]" -print 
.endif

TAGS: ${SWALNUT} ${COMM} ${SPPC} ${COMM} ${ASPARC64}
	@echo Making TAGS...
	@${ETAGS} ${SWALNUT} ${COMM} ${SPPC} ${COMM} "--regex=/^GLOBAL(\(.*\))/\1/" \
		"--regex=/^_C_LABEL(\(.*\)):/\1/" "--regex=/^\(.*\):/\1/" \
		"--regex=/^ENTRY(\(.*\)):/\1/" "--regex=/^ASENTRY(\(.*\)):/\1/" \
		${APPC} ${AWALNUT}

.include <bsd.subdir.mk>
