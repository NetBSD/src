#	$NetBSD: Makefile,v 1.1.1.1 1996/03/09 00:01:12 phil Exp $
# Makefile for g++ library genclass 

PROG=		genclass
SRCS=	genclass.sh 
NOMAN=
STRIP=

SRCDIR=${.CURDIR}

BINDIR ?= /usr/bin

genclass:	genclass.sh
	cp $(SRCDIR)/genclass.sh genclass

.include <bsd.prog.mk>

