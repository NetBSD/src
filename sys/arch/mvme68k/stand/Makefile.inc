#	$NetBSD: Makefile.inc,v 1.4 2000/07/09 13:47:33 jdolecek Exp $

MDEC_DIR?=/usr/mdec

CLEANFILES+= machine m68k

.if defined(S)
machine:
	-rm -f $@
	ln -s $S/arch/mvme68k/include machine

m68k:
	-rm -f $@
	ln -s $S/arch/m68k/include m68k

all: machine m68k
.endif
