#	$NetBSD: gcc-version.mk,v 1.27 2024/06/30 09:41:47 mrg Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 10
NETBSD_GCC_VERSION=nb3 20231008
.endif
.if ${HAVE_GCC} == 12
NETBSD_GCC_VERSION=nb1 20240630
.endif
