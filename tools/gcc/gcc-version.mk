#	$NetBSD: gcc-version.mk,v 1.12 2018/11/11 23:05:25 mrg Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 6
NETBSD_GCC_VERSION=nb4 20181109
.elif ${HAVE_GCC} == 7
NETBSD_GCC_VERSION=nb1 20180905
.endif
