#	$NetBSD: Makefile,v 1.2 2003/01/06 17:40:28 lukem Exp $

ETAGS?= etags

# XXX Fix this to deal with multiple "evbppc" ports.
SEVBPPC=	evbppc/*.[ch] include/*.h
AEVBPPC=

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

TAGS: ${SEVBPPC} ${COMM} ${SPPC} ${COMM} ${ASPARC64}
	@echo Making TAGS...
	@${ETAGS} ${SEVBPPC} ${COMM} ${SPPC} ${COMM} "--regex=/^GLOBAL(\(.*\))/\1/" \
		"--regex=/^_C_LABEL(\(.*\)):/\1/" "--regex=/^\(.*\):/\1/" \
		"--regex=/^ENTRY(\(.*\)):/\1/" "--regex=/^ASENTRY(\(.*\)):/\1/" \
		${APPC} ${AEVBPPC}


SUBDIR=	compile include

.include <bsd.subdir.mk>
