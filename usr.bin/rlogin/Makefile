#	$NetBSD: Makefile,v 1.13 1999/07/12 22:11:38 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

SRCTOP=	../..
.include <bsd.crypto.mk>

PROG=	rlogin
SRCS=	rlogin.c

BINOWN=	root
BINMODE=4555
#INSTALLFLAGS=-fschg

.if defined(CRYPTOPATH)
.include "${CRYPTOPATH}/usr.bin/rlogin/Makefile.frag"
.endif

.include <bsd.prog.mk>
