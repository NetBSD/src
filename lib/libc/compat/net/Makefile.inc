#	$NetBSD: Makefile.inc,v 1.1.42.1 2012/04/17 00:05:17 yamt Exp $

.PATH: ${COMPATDIR}/net
SRCS+=	__cmsg_alignbytes.c compat_ns_addr.c compat_ns_ntoa.c
