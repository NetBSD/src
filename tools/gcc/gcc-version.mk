#	$NetBSD: gcc-version.mk,v 1.20 2020/09/06 21:59:33 mrg Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 8
NETBSD_GCC_VERSION=nb1 20200311
.elif ${HAVE_GCC} == 9
NETBSD_GCC_VERSION=nb1 20200907
.endif
