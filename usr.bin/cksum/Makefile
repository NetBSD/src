#	$NetBSD: Makefile,v 1.7 1997/01/30 01:10:32 thorpej Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/28/95

PROG=	cksum
SRCS=	cksum.c crc.c md5.c print.c sum1.c sum2.c
LINKS=	${BINDIR}/cksum ${BINDIR}/sum
LINKS+=	${BINDIR}/cksum ${BINDIR}/md5
MLINKS=	cksum.1 sum.1
MLINKS+=cksum.1 md5.1

.include <bsd.prog.mk>
