#	$NetBSD: Makefile,v 1.8 1999/01/26 22:45:02 tron Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 5/23/94

DIR=	psd/19.curses
SRCS=	Master
MACROS=	-me
OTHER=	appen.A appen.B appen.C doc.I doc.II doc.III doc.IV fns.doc \
	intro.0 intro.1 intro.3 intro.4 intro.5 intro.6 \
	macros c_macros
CEXAMPLES= ex1.gr ex2.gr life.gr twinkle1.gr twinkle2.gr win_st.gr
TBLFILES=  intro.2.tbl
EXTRA=	${OTHER} ${CEXAMPLES:R:S/$/.c/g} ${TBLFILES:R}

CLEANFILES+= ${CEXAMPLES} ${TBLFILES}

.SUFFIXES:
.SUFFIXES: .c .gr

#
# this section formats C input source into nice troffable (or nroffable)
# versions.  It uses the capabilites of "vgrind", which sets keywords in
# bold font, and comments in italics.
#

# Don't re-run vgrind unless you want to patch the output files.
VFONT=	/usr/libexec/vfontedpr
.c.gr:
	${VFONT} ${.IMPSRC} | grep -v "^'wh" > ${.TARGET}

paper.ps: ${SRCS} ${OTHER} ${CEXAMPLES} ${TBLFILES}
	${SOELIM} -I${.CURDIR} Master | ${ROFF} > ${.TARGET}

# Unfortunately our make doesn't handle single-suffix rules.
intro.2.tbl: intro.2
	${TBL} ${.ALLSRC} > ${.TARGET}

.include <bsd.doc.mk>
