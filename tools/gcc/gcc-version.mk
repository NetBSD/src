#	$NetBSD: gcc-version.mk,v 1.5 2016/03/17 23:41:21 mrg Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 48
NETBSD_GCC_VERSION=nb3 20151015
.else
NETBSD_GCC_VERSION=nb1 20160317
.endif
