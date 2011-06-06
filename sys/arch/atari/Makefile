#	$NetBSD: Makefile,v 1.7.22.1 2011/06/06 09:05:07 jruoho Exp $

# Makefile for atari tags file and boot blocks

# Find where m68k source files are for inclusion in tags
.include <../m68k/Makefile.inc>

TATARI=	${SYSDIR}/arch/atari/tags
SATARI=		${SYSDIR}/arch/atari/atari/*.[ch]
SATARI+=	${SYSDIR}/arch/atari/include/*.h
SATARI+=	${SYSDIR}/arch/atari/dev/*.[ch]
SATARI+=	${SYSDIR}/arch/atari/isa/*.[ch]
SATARI+=	${SYSDIR}/arch/atari/pci/*.[ch]
SATARI+=	${SYSDIR}/arch/atari/vme/*.[ch]

AATARI=		${SYSDIR}/arch/atari/atari/*.s
AATARI+=	${SYSDIR}/arch/atari/dev/*.s

# Directories in which to place tags links
DATARI=	atari dev include

.include "../../kern/Make.tags.inc"

tags:
	-rm -f ${TATARI}
	-echo ${SATARI} ${SM68K} | xargs ctags -wadtf ${TATARI}
	-${FINDCOMM} | xargs ctags -wadtf ${TATARI}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AATARI} ${AM68K} | \
	    ${TOOL_SED} "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TATARI}
	sort -o ${TATARI} ${TATARI}

links:
	-for i in ${DATARI}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	compile dev include stand

.include <bsd.subdir.mk>
