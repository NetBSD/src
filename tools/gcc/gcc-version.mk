#	$NetBSD: gcc-version.mk,v 1.15 2019/02/27 09:11:01 mrg Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 6
NETBSD_GCC_VERSION=nb4 20181109
.elif ${HAVE_GCC} == 7
NETBSD_GCC_VERSION=nb2 20190226
.endif
