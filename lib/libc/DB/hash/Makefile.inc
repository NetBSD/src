#       @(#)Makefile.inc	5.2 (Berkeley) 9/4/91

.PATH: ${.CURDIR}/DB/hash

SRCS+=	hash.c hash_bigkey.c hash_buf.c hash_func.c hash_log2.c \
	hash_page.c hsearch.c ndbm.c
