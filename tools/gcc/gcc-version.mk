#	$NetBSD: gcc-version.mk,v 1.10.2.1 2019/06/10 22:10:14 christos Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 6
NETBSD_GCC_VERSION=nb4 20181109
.elif ${HAVE_GCC} == 7
NETBSD_GCC_VERSION=nb3 20190319
.endif
