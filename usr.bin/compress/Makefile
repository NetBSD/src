#	$NetBSD: Makefile,v 1.11 1998/10/08 01:56:28 wsanchez Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/17/94

PROG=    compress
SRCS=    compress.c zopen.c
LINKS=   ${BINDIR}/compress ${BINDIR}/uncompress
#LINKS+=  ${BINDIR}/compress ${BINDIR}/zcat
MLINKS=  compress.1 uncompress.1
#MLINKS+= compress.1 zcat.1

.include <bsd.prog.mk>
