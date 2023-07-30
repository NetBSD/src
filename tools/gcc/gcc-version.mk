#	$NetBSD: gcc-version.mk,v 1.24 2023/07/30 06:36:21 mrg Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 10
NETBSD_GCC_VERSION=nb2 20230710
.endif
.if ${HAVE_GCC} == 12
NETBSD_GCC_VERSION=nb1 20230729
.endif
