#	$NetBSD: Makefile,v 1.3.22.1 2011/06/06 09:04:41 jruoho Exp $

# Makefile for algor tags file and boot blocks

# Find where mips source files are for inclusion in tags
.include <../mips/Makefile.inc>

TALGOR=		${SYSDIR}/arch/algor/tags
SALGOR=		${SYSDIR}/arch/algor/algor/*.[ch]
SALGOR+=	${SYSDIR}/arch/algor/dev/*.[ch]
SALGOR+=	${SYSDIR}/arch/algor/include/*.h
SALGOR+=	${SYSDIR}/arch/algor/isa/*.[ch]
SALGOR+=	${SYSDIR}/arch/algor/pci/*.[ch]

AALGOR=		${SYSDIR}/arch/algor/algor/*.S
# Directories in which to place tags links
DALGOR= algor include isa pci

.include "../../kern/Make.tags.inc"

tags:
	-rm -f ${TALGOR}
	-echo ${SALGOR} ${SMIPS} | xargs ctags -wadtf ${TALGOR}
	-${FINDCOMM} | xargs ctags -wadtf ${TALGOR}
	egrep "^LEAF(.*)|^[AN]LEAF(.*)|^NON_LEAF(.*)" ${AALGOR} ${AMIPS} | \
	    ${TOOL_SED} -e \
		"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3	\1	/^\2(\3\4$$/;" \
	    >> ${TALGOR}
	sort -o ${TALGOR} ${TALGOR}

links:
	-for i in ${DALGOR}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	compile include

.include <bsd.subdir.mk>
