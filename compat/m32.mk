#	$NetBSD: m32.mk,v 1.2 2011/06/18 01:22:34 mrg Exp $

#
# Makefile fragment to help implement a set of 'cc -m32' libraries.
#

.ifndef _COMPAT_M32_MK_ # {
_COMPAT_M32_MK_=1

COPTS+=			-m32
CPUFLAGS+=		-m32
LDADD+=			-m32
LDFLAGS+=		-m32
MKDEPFLAGS+=		-m32

.include "Makefile.compat"

.endif # _COMPAT_M32_MK_ }
