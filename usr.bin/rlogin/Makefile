#	$NetBSD: Makefile,v 1.14 1999/07/20 09:35:21 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

SRCTOP=	../..
.include <bsd.crypto.mk>

PROG=	rlogin
SRCS=	rlogin.c

BINOWN=	root
BINMODE=4555
#INSTALLFLAGS=-fschg

.if defined(CRYPTOPATH)
.sinclude "${CRYPTOPATH}/usr.bin/rlogin/Makefile.frag"
.endif

.include <bsd.prog.mk>
