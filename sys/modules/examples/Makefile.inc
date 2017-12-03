#	$NetBSD: Makefile.inc,v 1.1.20.2 2017/12/03 11:38:53 jdolecek Exp $

# We only build the example modules; we don't want to install them anywhere

kmodinstall:

# Include parent's Makefile.inc

.include "../Makefile.inc"
