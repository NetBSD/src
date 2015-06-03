#	$NetBSD: Makefile.inc,v 1.1.2.2 2015/06/03 06:51:40 snj Exp $

# We only build the example modules; we don't want to install them anywhere

kmodinstall:

# Include parent's Makefile.inc

.include "../Makefile.inc"
