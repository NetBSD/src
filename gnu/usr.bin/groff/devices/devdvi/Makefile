#	$NetBSD: Makefile,v 1.8 2001/04/19 14:40:53 wiz Exp $

.include "../../Makefile.path"

.PATH:		${IIDIST}/font/devdvi

DEVICE=		dvi
FONTFILES=	R I B BI CW CWI MI S EX H HI HB SA SB DESC $(DEVGENFILES)
DEVGENFILES=	generate/CompileFonts generate/Makefile generate/msam.map\
		generate/msbm.map generate/texb.map generate/texex.map \
		generate/texi.map generate/texmi.map generate/texr.map \
		generate/texsy.map generate/textt.map

CLEANFILES=	DESC

DESC: DESC.in
	@cat $> >$@
	@test -z '$(DVIPRINT)' || echo print '$(DVIPRINT)' >>$@

.include "../Makefile.dev"
