#	$NetBSD: Makefile.inc,v 1.1.4.2 2015/06/06 14:40:23 skrll Exp $

# We only build the example modules; we don't want to install them anywhere

kmodinstall:

# Include parent's Makefile.inc

.include "../Makefile.inc"
