#	$NetBSD: gcc-version.mk,v 1.6 2016/06/07 08:12:13 mrg Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 48
NETBSD_GCC_VERSION=nb3 20151015
.else
NETBSD_GCC_VERSION=nb1 20160606
.endif
