#	$NetBSD: Makefile.inc,v 1.1 2015/05/13 07:07:36 pgoyette Exp $

# We only build the example modules; we don't want to install them anywhere

kmodinstall:

# Include parent's Makefile.inc

.include "../Makefile.inc"
