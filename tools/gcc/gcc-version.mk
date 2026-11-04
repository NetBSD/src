#	$NetBSD: gcc-version.mk,v 1.26.2.3 2026/11/04 23:28:29 perseant Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 12
NETBSD_GCC_VERSION=nb3 20260326
.endif
.if ${HAVE_GCC} == 14
NETBSD_GCC_VERSION=nb4 20260926
.endif
