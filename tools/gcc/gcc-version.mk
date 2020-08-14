#	$NetBSD: gcc-version.mk,v 1.16.2.1 2020/08/14 11:02:42 martin Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 7
NETBSD_GCC_VERSION=nb4 20200810
.endif
