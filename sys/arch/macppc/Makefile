#	$NetBSD: Makefile,v 1.3.2.2 2004/09/18 14:36:56 skrll Exp $

# Makefile for macppc tags file

TIMACPPC=	${SYSDIR}/arch/macppc/tags
SIMACPPC=	${SYSDIR}/arch/macppc/macppc/*.[ch] \
		${SYSDIR}/arch/macppc/include/*.h \
		${SYSDIR}/arch/macppc/dev/*.[ch] \
		${SYSDIR}/arch/macppc/pci/*.[ch] \
		${SYSDIR}/../contrib/sys/dev/ic/*.[ch]
AIMACPPC=	${SYSDIR}/arch/macppc/macppc/*.[sS]

# Directories in which to place tags links
DIMACPPC=	macppc dev include pci

.include "../../kern/Make.tags.inc"

tags:
	rm -f ${TIMACPPC}
	-echo ${SIMACPPC} ${COMM} | xargs ctags -wadtf ${TIMACPPC}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AIMACPPC} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
		>> ${TIMACPPC}
	sort -o ${TIMACPPC} ${TIMACPPC}

links:
	-for i in ${DIMACPPC}; do \
	    (cd $$i && rm -f tags; ln -s ../tags tags); done

SUBDIR=	compile include stand

.include <bsd.subdir.mk>
