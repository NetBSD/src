#	$NetBSD: Makefile,v 1.10 2001/03/20 18:46:25 atatat Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/28/95

PROG=	cksum
SRCS=	cksum.c crc.c md2.c md4.c md5.c sha1.c rmd160.c print.c sum1.c sum2.c
LINKS=	${BINDIR}/cksum ${BINDIR}/sum
LINKS+=	${BINDIR}/cksum ${BINDIR}/md2
LINKS+=	${BINDIR}/cksum ${BINDIR}/md4
LINKS+=	${BINDIR}/cksum ${BINDIR}/md5
LINKS+=	${BINDIR}/cksum ${BINDIR}/sha1
LINKS+=	${BINDIR}/cksum ${BINDIR}/rmd160
MLINKS=	cksum.1 sum.1
MLINKS+=cksum.1 md2.1
MLINKS+=cksum.1 md4.1
MLINKS+=cksum.1 md5.1
MLINKS+=cksum.1 sha1.1
MLINKS+=cksum.1 rmd160.1

.include <bsd.prog.mk>
