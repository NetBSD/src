#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93
#	$Id: Makefile,v 1.7 1993/09/16 23:18:45 cgd Exp $

PROG=	dbtest
SRCS=	dbtest.c
#SRCS+=	rec_close.c rec_delete.c rec_get.c rec_open.c rec_put.c \
#	rec_search.c rec_seq.c rec_utils.c
#.PATH:	${.CURDIR}/../recno
#SRCS+=	bt_close.c bt_conv.c bt_debug.c bt_delete.c bt_get.c bt_open.c \
#	bt_overflow.c bt_put.c bt_search.c bt_seq.c bt_split.c bt_stack.c \
#	bt_utils.c
#.PATH:	${.CURDIR}/../btree
#SRCS+=	hash.c hash_bigkey.c hash_buf.c hash_func.c hash_log2.c \
#	hash_page.c hsearch.c ndbm.c
#.PATH:	${.CURDIR}/../hash
#LDADD=	/usr/src/lib/libc/obj/libc.a

CFLAGS= -g -DDEBUG #-DSTATISTICS
NOMAN=  noman
CLEANFILES+= t1 t2 t3

install:

regress:
	sh ${.CURDIR}/run.test

.include <bsd.prog.mk>
