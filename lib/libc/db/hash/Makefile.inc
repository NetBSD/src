#	$NetBSD: Makefile.inc,v 1.7 2001/02/14 05:44:29 cgd Exp $
#       @(#)Makefile.inc	8.1 (Berkeley) 6/4/93

.PATH: ${.CURDIR}/db/hash

SRCS+=	hash.c hash_bigkey.c hash_buf.c hash_func.c hash_log2.c \
	hash_page.c ndbm.c
