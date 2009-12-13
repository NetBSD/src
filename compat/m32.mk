#	$NetBSD: m32.mk,v 1.1 2009/12/13 09:27:34 mrg Exp $

#
# Makefile fragment to help implement a set of 'cc -m32' libraries.
#

COPTS+=			-m32
CPUFLAGS+=		-m32
LDADD+=			-m32
LDFLAGS+=		-m32
MKDEPFLAGS+=		-m32

.include "Makefile.compat"
