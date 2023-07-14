#	$NetBSD: gcc-version.mk,v 1.22.2.1 2023/07/14 08:21:45 martin Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 10
NETBSD_GCC_VERSION=nb2 20230710
.endif
