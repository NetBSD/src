#	$NetBSD: Makefile,v 1.5 2008/01/10 07:34:02 dyoung Exp $

# Makefile for evbarm tags file and boot blocks

TEVBARM=	${SYSDIR}/arch/evbarm/tags
SEVBARM=	${SYSDIR}/arch/arm/xscale/*.[ch] \
		${SYSDIR}/arch/evbarm/dev/*.[ch] \
		${SYSDIR}/arch/evbarm/ifpga/*.[ch] \
		${SYSDIR}/arch/evbarm/include/*.h \
		${SYSDIR}/arch/evbarm/integrator/*.[ch] \
		${SYSDIR}/arch/evbarm/iq31244/*.[ch] \
		${SYSDIR}/arch/evbarm/iq80310/*.[ch] \
		${SYSDIR}/arch/evbarm/iq80321/*.[ch]
AEVBARM=	${SYSDIR}/arch/evbarm/integrator/*.S

# Directories in which to place tags links
DEVBARM=	include

.include "../../kern/Make.tags.inc"

tags:
	-rm -f ${TEVBARM}
	-echo ${SEVBARM} | xargs ctags -wadtf ${TEVBARM}
	-${FINDCOMM} | xargs ctags -wadtf ${TEVBARM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AEVBARM} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TEVBARM}
	sort -o ${TEVBARM} ${TEVBARM}

links:
	-for i in ${DEVBARM}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	compile include stand

.include <bsd.subdir.mk>
