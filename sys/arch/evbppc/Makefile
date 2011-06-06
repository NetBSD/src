#	$NetBSD: Makefile,v 1.2.136.1 2011/06/06 09:05:31 jruoho Exp $

.include <../powerpc/Makefile.inc>

# XXX Fix this to deal with multiple "evbppc" ports.
TEVBPPC=	${SYSDIR}/arch/evbppc/tags
SEVBPPC=	${SYSDIR}/arch/evbppc/evbppc/*.[ch]
SEVBPPC+=	${SYSDIR}/arch/evbppc/include/*.h
SEVBPPC+=	${SYSDIR}/arch/evbppc/pmppc/dev/*.[ch]
SEVBPPC+=	${SYSDIR}/arch/evbppc/pmppc/pci/*.[ch]
SEVBPPC+=	${SYSDIR}/arch/evbppc/pmppc/*.[ch]
AEVBPPC=	${SYSDIR}/arch/evbppc/ev64260/*.S \
		${SYSDIR}/arch/evbppc/explora/*.S \
		${SYSDIR}/arch/evbppc/mpc85xx/*.S \
		${SYSDIR}/arch/evbppc/obs405/*.S \
		${SYSDIR}/arch/evbppc/pmppc/*.S \
		${SYSDIR}/arch/evbppc/virtex/*.S \
		${SYSDIR}/arch/evbppc/walnut/*.S

DEVBPPC=	include

.include "../../kern/Make.tags.inc"

tags:
	-rm -f ${TEVBPPC}
	-echo ${SEVBPPC} ${SPPC} | xargs ctags -wadtf ${TEVBPPC}
	-${FINDCOMM} | xargs ctags -wadtf ${TEVBPPC}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AEVBPPC} ${APPC} | \
	    ${TOOL_SED} -e \
		"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3	\1	/^\2(\3\4$$/;" \
	    >> ${TEVBPPC}
	sort -o ${TEVBPPC} ${TEVBPPC}

links:
	-for i in ${DEVBPPC}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	compile include

.include <bsd.subdir.mk>
