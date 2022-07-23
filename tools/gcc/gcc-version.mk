#	$NetBSD: gcc-version.mk,v 1.22 2022/07/23 19:01:18 mrg Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 8
NETBSD_GCC_VERSION=nb1 20200311
.elif ${HAVE_GCC} == 9
NETBSD_GCC_VERSION=nb1 20200907
.elif ${HAVE_GCC} == 10
NETBSD_GCC_VERSION=nb1 20220722
.endif
