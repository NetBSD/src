#	$NetBSD: gcc-version.mk,v 1.9.2.4 2019/01/26 22:00:38 pgoyette Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 6
NETBSD_GCC_VERSION=nb4 20181109
.elif ${HAVE_GCC} == 7
NETBSD_GCC_VERSION=nb1 20190119
.endif
