#	$NetBSD: Makefile,v 1.2 1996/12/20 01:04:20 cgd Exp $
# Makefile for g++ library genclass 

PROG=		genclass
SRCS=	genclass.sh 
NOMAN=
NOLINT=
STRIP=

SRCDIR=${.CURDIR}

BINDIR ?= /usr/bin

genclass:	genclass.sh
	cp $(SRCDIR)/genclass.sh genclass

.include <bsd.prog.mk>

