#	$NetBSD: gcc-version.mk,v 1.9.2.1 2018/03/30 06:20:17 pgoyette Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 5
NETBSD_GCC_VERSION=nb2 20180327
.elif ${HAVE_GCC} == 6
NETBSD_GCC_VERSION=nb2 20180327
.endif
