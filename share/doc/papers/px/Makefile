#	$NetBSD: Makefile,v 1.4.10.1 2001/03/11 19:09:38 hubertf Exp $
#
#	@(#)Makefile	5.4 (Berkeley) 10/9/94

DIR=	papers/px
SRCS=	tmac.p pxin0.n pxin1.n pxin2.n pxin3.n pxin4.n
MACROS=	-ms
EXTRA=	fig1.1.n fig1.2.n fig1.3.n fig2.3.raw fig2.4.n fig3.2.n \
	fig3.3.n table2.1.n table2.2.n table2.3.n table3.1.n tmac.p
CLEANFILES+=fig2.3.n

paper.ps: ${SRCS} fig2.3.n
	${SOELIM} ${SRCS} | ${TBL} | ${ROFF} > ${.TARGET}

fig2.3.n: fig2.3.raw
	sort fig2.3.raw >fig2.3.n

.include <bsd.doc.mk>
