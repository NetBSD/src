#	$NetBSD: gcc-version.mk,v 1.26 2024/02/25 02:24:19 mrg Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 10
NETBSD_GCC_VERSION=nb3 20231008
.endif
.if ${HAVE_GCC} == 12
NETBSD_GCC_VERSION=nb2 20240221
.endif
