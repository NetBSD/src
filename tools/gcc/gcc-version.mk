#	$NetBSD: gcc-version.mk,v 1.17 2019/09/30 08:40:20 mrg Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 7
NETBSD_GCC_VERSION=nb3 20190319
.elif ${HAVE_GCC} == 8
NETBSD_GCC_VERSION=nb1 20190930
.endif
