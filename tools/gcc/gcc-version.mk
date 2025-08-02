#	$NetBSD: gcc-version.mk,v 1.26.2.2 2025/08/02 05:58:23 perseant Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 10
NETBSD_GCC_VERSION=nb3 20231008
.endif
.if ${HAVE_GCC} == 12
NETBSD_GCC_VERSION=nb1 20250721
.endif
