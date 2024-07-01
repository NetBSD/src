#	$NetBSD: gcc-version.mk,v 1.26.2.1 2024/07/01 01:01:14 perseant Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 10
NETBSD_GCC_VERSION=nb3 20231008
.endif
.if ${HAVE_GCC} == 12
NETBSD_GCC_VERSION=nb1 20240630
.endif
