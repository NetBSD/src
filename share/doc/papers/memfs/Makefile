#	$NetBSD: Makefile,v 1.4 2003/07/10 10:34:27 lukem Exp $
#
#	@(#)Makefile	1.8 (Berkeley) 6/8/93

DIR=	papers/memfs
SRCS=	0.t 1.t
MACROS=	-ms
EXTRA=	ref.bib A.t tmac.srefs
CLEANFILES=ref.bib.i A.gt paper.t

paper.ps: paper.t
	${TOOL_ROFF_PS} ${MACROS} tmac.srefs paper.t > ${.TARGET}

paper.t: ${SRCS} ref.bib.i A.gt
	${TOOL_REFER} -n -e -l -s -p ref.bib ${SRCS} A.gt > ${.TARGET}

ref.bib.i: ref.bib
	${TOOL_INDXBIB} ref.bib

A.gt: A.t
	${TOOL_VGRIND} < A.t > A.gt

.include <bsd.doc.mk>
