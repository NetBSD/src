#	$NetBSD: Makefile.inc,v 1.8 2004/04/28 00:10:52 kleink Exp $
#       @(#)Makefile.inc	8.1 (Berkeley) 6/4/93

.PATH: ${.CURDIR}/db/hash

SRCS+=	hash.c hash_bigkey.c hash_buf.c hash_func.c hash_log2.c \
	hash_page.c __ndbm13.c ndbm.c
