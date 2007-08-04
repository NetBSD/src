#	$NetBSD: Makefile,v 1.4 2007/08/04 05:32:39 dyoung Exp $

# Makefile for sgimips tags file

# Find where mips source files are for inclusion in tags
.include <../mips/Makefile.inc>

TSGIMIPS=	${SYSDIR}/arch/sgimips/tags
SSGIMIPS=	${SYSDIR}/arch/sgimips/sgimips/*.[ch] \
		${SYSDIR}/arch/sgimips/include/*.h \
		${SYSDIR}/arch/sgimips/dev/*.[ch] \
		${SYSDIR}/arch/sgimips/eisa/*.[ch] \
		${SYSDIR}/arch/sgimips/gio/*.[ch] \
		${SYSDIR}/arch/sgimips/hpc/*.[ch] \
		${SYSDIR}/arch/sgimips/ioc/*.[ch] \
		${SYSDIR}/arch/sgimips/mace/*.[ch] \
		${SYSDIR}/arch/sgimips/pci/*.[ch]
ASGIMIPS=	${SYSDIR}/arch/sgimips/sgimips/*.S

# Directories in which to place tags links
DSGIMIPS=	dev eisa gio hpc include pci xio

.include "../../kern/Make.tags.inc"

tags:
	-rm -f ${TSGIMIPS}
	-echo ${SSGIMIPS} ${SMIPS} | xargs ctags -wadtf ${TSGIMIPS}
	-${FINDCOMM} | xargs ctags -wadtf ${TSGIMIPS}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${ASGIMIPS} ${AMIPS} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TSGIMIPS}
	sort -o ${TSGIMIPS} ${TSGIMIPS}

links:
	-for i in ${DSGIMIPS}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	compile include stand

.include <bsd.subdir.mk>
