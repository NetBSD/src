#	$NetBSD: Makefile,v 1.2.2.2 2001/11/12 21:16:48 thorpej Exp $

# Makefile for evbarm tags file and boot blocks

TEVBARM=	../evbarm/tags
SEVBARM=	../evbarm/integrator/*.[ch] ../evbarm/include/*.h \
		../evbarm/dev/*.[ch] ../evbarm/ifpga/*.[ch] \
		../evbarm/iq80310/*.[ch] \
		../evbarm/integrator_fpga/*.[ch] \
AEVBARM=	../evbarm/integrator/*.S ../evbarm/ifpga/*.S \

# Directories in which to place tags links
DEVBARM=	include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TEVBARM} ${SEVBARM} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AEVBARM} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TEVBARM}
	sort -o ${TEVBARM} ${TEVBARM}

links:
	-for i in ${DEVBARM}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	include

.include <bsd.subdir.mk>
