#	$NetBSD: gcc-version.mk,v 1.16 2019/03/20 05:09:26 mrg Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 6
NETBSD_GCC_VERSION=nb4 20181109
.elif ${HAVE_GCC} == 7
NETBSD_GCC_VERSION=nb3 20190319
.endif
