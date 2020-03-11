#	$NetBSD: gcc-version.mk,v 1.18 2020/03/11 10:07:01 mrg Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 7
NETBSD_GCC_VERSION=nb3 20190319
.elif ${HAVE_GCC} == 8
NETBSD_GCC_VERSION=nb1 20200311
.endif
