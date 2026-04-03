#	$NetBSD: gcc-version.mk,v 1.28.2.2 2026/04/03 13:28:19 martin Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 12
NETBSD_GCC_VERSION=nb3 20260326
.endif
