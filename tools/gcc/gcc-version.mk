#	$NetBSD: gcc-version.mk,v 1.28 2025/07/21 05:27:09 mrg Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 10
NETBSD_GCC_VERSION=nb3 20231008
.endif
.if ${HAVE_GCC} == 12
NETBSD_GCC_VERSION=nb1 20250721
.endif
