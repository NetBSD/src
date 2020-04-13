#	$NetBSD: gcc-version.mk,v 1.10.2.2 2020/04/13 08:05:34 martin Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 7
NETBSD_GCC_VERSION=nb3 20190319
.elif ${HAVE_GCC} == 8
NETBSD_GCC_VERSION=nb1 20200311
.endif
