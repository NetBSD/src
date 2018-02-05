#	$NetBSD: gcc-version.mk,v 1.9 2018/02/05 06:22:27 mrg Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 5
NETBSD_GCC_VERSION=nb1 20171112
.elif ${HAVE_GCC} == 6
NETBSD_GCC_VERSION=nb1 20180203
.endif
