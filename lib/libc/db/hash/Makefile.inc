#	$NetBSD: Makefile.inc,v 1.9.98.1 2026/11/04 23:19:16 perseant Exp $
#       @(#)Makefile.inc	8.1 (Berkeley) 6/4/93

.PATH: ${.PARSEDIR}

SRCS+=	hash.c hash_bigkey.c hash_buf.c hash_func.c hash_log2.c \
	hash_page.c ndbmdatum.c ndbm.c
