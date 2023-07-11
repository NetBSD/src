#	$NetBSD: gcc-version.mk,v 1.23 2023/07/11 18:13:27 mrg Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 10
NETBSD_GCC_VERSION=nb2 20230710
.endif
