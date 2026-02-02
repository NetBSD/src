#	$NetBSD: gcc-version.mk,v 1.28.2.1 2026/02/02 20:31:18 martin Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 12
NETBSD_GCC_VERSION=nb1 20260119
.endif
