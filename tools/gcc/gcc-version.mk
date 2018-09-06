#	$NetBSD: gcc-version.mk,v 1.9.2.2 2018/09/06 06:56:49 pgoyette Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 6
NETBSD_GCC_VERSION=nb3 20180905
.elif ${HAVE_GCC} == 7
NETBSD_GCC_VERSION=nb1 20180905
.endif
