#	$NetBSD: gcc-version.mk,v 1.22.2.2 2023/10/09 07:38:16 martin Exp $

# common location for tools and native build

.if ${HAVE_GCC} == 10
NETBSD_GCC_VERSION=nb3 20231008
.endif
