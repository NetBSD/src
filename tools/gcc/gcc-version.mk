#	$NetBSD: gcc-version.mk,v 1.31 2026/02/02 08:36:48 mrg Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 12
NETBSD_GCC_VERSION=nb2 20260119
.endif
.if ${HAVE_GCC} == 14
NETBSD_GCC_VERSION=nb2 20260118
.endif
