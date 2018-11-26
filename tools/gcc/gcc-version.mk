#	$NetBSD: gcc-version.mk,v 1.9.2.3 2018/11/26 01:52:53 pgoyette Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 6
NETBSD_GCC_VERSION=nb4 20181109
.elif ${HAVE_GCC} == 7
NETBSD_GCC_VERSION=nb1 20180905
.endif
