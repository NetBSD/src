#	$NetBSD: Makefile,v 1.5 1996/05/24 23:18:23 pk Exp $

# from: @(#)Makefile	8.1 (Berkeley) 6/11/93
#
# Makefile for sparc links, tags file

SUBDIR= stand

NOPROG=	noprog
NOMAN=	noman
NOOBJ=	noobj

DIRS=	conf dev fpu include sparc

links::
	-for i in ${DIRS}; do \
	    (cd $$i && { rm -f tags; ln -s ${SYSTAGS} tags; }) done

SPARC=	/sys/arch/sparc/dev/*.[ch] /sys/arch/sparc/fpu/*.[ch] \
	/sys/arch/sparc/include/*.[ch] /sys/arch/sparc/sparc/*.[ch]
ASPARC=	/sys/arch/sparc/sparc/*.s

tags:
	-ctags -wdt ${COMM} ${SPARC}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${ASPARC} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
		>> tags
	sort -o tags tags

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
