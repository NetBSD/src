#	$NetBSD: Makefile.inc,v 1.10 2026/10/04 03:28:42 mrg Exp $
#       @(#)Makefile.inc	8.1 (Berkeley) 6/4/93

.PATH: ${.PARSEDIR}

SRCS+=	hash.c hash_bigkey.c hash_buf.c hash_func.c hash_log2.c \
	hash_page.c ndbmdatum.c ndbm.c
