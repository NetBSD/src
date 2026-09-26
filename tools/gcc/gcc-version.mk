#	$NetBSD: gcc-version.mk,v 1.34 2026/09/26 23:33:53 mrg Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 12
NETBSD_GCC_VERSION=nb3 20260326
.endif
.if ${HAVE_GCC} == 14
NETBSD_GCC_VERSION=nb4 20260926
.endif
