#	$NetBSD: gcc-version.mk,v 1.33 2026/04/28 23:32:08 mrg Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 12
NETBSD_GCC_VERSION=nb3 20260326
.endif
.if ${HAVE_GCC} == 14
NETBSD_GCC_VERSION=nb3 20260428
.endif
