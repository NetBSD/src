#	$NetBSD: gcc-version.mk,v 1.30 2026/01/19 22:02:23 mrg Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 12
NETBSD_GCC_VERSION=nb1 20250721
.endif
.if ${HAVE_GCC} == 14
NETBSD_GCC_VERSION=nb2 20260118
.endif
