#	$NetBSD: gcc-version.mk,v 1.11 2018/09/05 05:03:28 mrg Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 6
NETBSD_GCC_VERSION=nb3 20180905
.elif ${HAVE_GCC} == 7
NETBSD_GCC_VERSION=nb1 20180905
.endif
