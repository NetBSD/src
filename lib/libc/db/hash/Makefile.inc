#	$NetBSD: Makefile.inc,v 1.6.18.1 2001/03/13 21:20:18 he Exp $
#       @(#)Makefile.inc	8.1 (Berkeley) 6/4/93

.PATH: ${.CURDIR}/db/hash

SRCS+=	hash.c hash_bigkey.c hash_buf.c hash_func.c hash_log2.c \
	hash_page.c ndbm.c
