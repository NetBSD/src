#	$NetBSD: Makefile.inc,v 1.3 2024/06/07 20:50:13 riastradh Exp $

.PATH: ${COMPATDIR}/net
SRCS+=	__cmsg_alignbytes.c compat_ns_addr.c compat_ns_ntoa.c

# For:
#
#	char *p; ... *p += ('A' - 'a')
#
# lint complains:
#
#	.../compat_ns_ntoa.c(98): warning: conversion of negative constant to unsigned type [222]
#
# when *p is 'a', 'b', 'c', 'd', 'e', or 'f'.
LINTFLAGS.compat_ns_ntoa.c+=	-X 222
