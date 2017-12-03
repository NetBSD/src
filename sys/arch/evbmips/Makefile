#	$NetBSD: Makefile,v 1.8.14.1 2017/12/03 11:36:08 jdolecek Exp $

# Makefile for evbmips tags file and boot blocks

# Find where mips source files are for inclusion in tags
.include <../mips/Makefile.inc>

# XXX !  fix this
TEVBMIPS=	${SYSDIR}/arch/evbmips/tags
SEVBMIPS=	${SYSDIR}/arch/evbmips/adm5120/*.[ch]
SEVBMIPS+=	${SYSDIR}/arch/evbmips/alchemy/*.[ch]
SEVBMIPS+=	${SYSDIR}/arch/evbmips/atheros/*.[ch]
SEVBMIPS+=	${SYSDIR}/arch/evbmips/evbmips/*.[ch]
SEVBMIPS+=	${SYSDIR}/arch/evbmips/gdium/*.[ch]
SEVBMIPS+=	${SYSDIR}/arch/evbmips/include/*.h
SEVBMIPS+=	${SYSDIR}/arch/evbmips/isa/*.[ch]
SEVBMIPS+=	${SYSDIR}/arch/evbmips/malta/*.[ch]
SEVBMIPS+=	${SYSDIR}/arch/evbmips/rmixl/*.[ch]
SEVBMIPS+=	${SYSDIR}/arch/evbmips/sbmips/*.[ch]
AEVBMIPS=	${SYSDIR}/arch/evbmips/evbmips/*.S
AEVBMIPS+=	${SYSDIR}/arch/evbmips/sbmips/*.S

# Directories in which to place tags links
DEVBMIPS= evbmips include isa pci

.include "../../kern/Make.tags.inc"

tags:
	-rm -f ${TEVBMIPS}
	-echo ${SEVBMIPS} ${SMIPS} | xargs ctags -wadtf ${TEVBMIPS}
	-${FINDCOMM} | xargs ctags -wadtf ${TEVBMIPS}
	egrep "^LEAF(.*)|^[AN]LEAF(.*)|^NON_LEAF(.*)" ${AEVBMIPS} ${AMIPS} | \
	    ${TOOL_SED} -e \
		"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3	\1	/^\2(\3\4$$/;" \
	    >> ${TEVBMIPS}
	sort -o ${TEVBMIPS} ${TEVBMIPS}

links:
	-for i in ${DEVBMIPS}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	compile include stand

.include <bsd.subdir.mk>
