#	$NetBSD: Makefile,v 1.5 2003/07/10 10:34:29 lukem Exp $
#
#	@(#)Makefile	1.6 (Berkeley) 6/8/93

DIR=	papers/relengr
SRCS=	0.t 1.t 2.t 3.t
MACROS=	-ms
EXTRA=	ref.bib tmac.srefs

paper.ps: ${SRCS}
	${TOOL_REFER} -n -e -l -s -p ref.bib ${SRCS} | \
	    ${TOOL_ROFF_PS} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
