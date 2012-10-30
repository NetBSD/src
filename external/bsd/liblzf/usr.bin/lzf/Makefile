#       $NetBSD: Makefile,v 1.1.2.2 2012/10/30 18:55:46 yamt Exp $

.include <bsd.own.mk>

PROG=	lzf
MAN=	lzf.1
SRCS=	lzf.c

LINKS+=	${BINDIR}/lzf ${BINDIR}/unlzf \
	${BINDIR}/lzf ${BINDIR}/lzfcat

MLINKS+= lzf.1 unlzf.1 \
	 lzf.1 lzfcat.1

LDADD=	-llzf

.include "${.CURDIR}/../Makefile.inc"

.include <bsd.prog.mk>
