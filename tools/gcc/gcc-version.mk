#	$NetBSD: gcc-version.mk,v 1.25 2023/10/08 21:52:34 mrg Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 10
NETBSD_GCC_VERSION=nb3 20231008
.endif
.if ${HAVE_GCC} == 12
NETBSD_GCC_VERSION=nb1 20230729
.endif
