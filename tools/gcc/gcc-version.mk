#	$NetBSD: gcc-version.mk,v 1.29 2025/09/16 02:08:01 mrg Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 12
NETBSD_GCC_VERSION=nb1 20250721
.endif
.if ${HAVE_GCC} == 14
NETBSD_GCC_VERSION=nb1 20250915
.endif
