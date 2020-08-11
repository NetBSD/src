#	$NetBSD: gcc-version.mk,v 1.19 2020/08/11 09:51:57 mrg Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 7
NETBSD_GCC_VERSION=nb4 20200810
.elif ${HAVE_GCC} == 8
NETBSD_GCC_VERSION=nb1 20200311
.endif
