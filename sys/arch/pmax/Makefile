#	$NetBSD: Makefile,v 1.6 1997/05/07 17:29:00 mycroft Exp $
#	@(#)Makefile	7.3 (Berkeley) 6/9/91

# Makefile for pmax tags file and boot blocks

NOPROG=	noprog
NOMAN=	noman

# Find where mips source fils are for inclusion in tags
.include <../mips/Makefile.inc>

SUBDIR=	stand

TPMAX=	../pmax/tags
SPMAX=	../pmax/pmax/*.[ch] ../pmax/include/*.h \
	../pmax/dev/*.[ch] ../pmax/tc/*.[ch]
APMAX=	../pmax/pmax/*.S

# Directories in which to place pmax (mips) tags links
DPMAX=	dev tc include

tags:	${COMM} ${SPMAX} ${SMIPS} ${APMAX} ${AMIPS}
	-ctags -dtf ${TPMAX} ${COMM} ${SPMAX} ${SMIPS}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${APMAX} ${AMIPS} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
		>> ${TPMAX}
	sort -o ${TPMAX} ${TPMAX}

links:
	-for i in ${DPMAX}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done

.include <bsd.prog.mk>
