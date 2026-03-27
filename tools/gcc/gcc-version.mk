#	$NetBSD: gcc-version.mk,v 1.32 2026/03/27 00:29:30 mrg Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 12
NETBSD_GCC_VERSION=nb3 20260326
.endif
.if ${HAVE_GCC} == 14
NETBSD_GCC_VERSION=nb2 20260118
.endif
