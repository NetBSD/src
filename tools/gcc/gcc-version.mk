#	$NetBSD: gcc-version.mk,v 1.21 2021/04/11 23:55:47 mrg Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 8
NETBSD_GCC_VERSION=nb1 20200311
.elif ${HAVE_GCC} == 9
NETBSD_GCC_VERSION=nb1 20200907
.elif ${HAVE_GCC} == 10
NETBSD_GCC_VERSION=nb1 20210411
.endif
