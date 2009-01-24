#	$NetBSD: Makefile,v 1.19 2009/01/24 22:11:45 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

NOMAN=  	# defined
WARNS=4

PROG=		dbtest
.if defined(LOCALLIBC)
SRCS=		dbtest.c
SRCS+=		hash.c hash_bigkey.c hash_buf.c hash_func.c hash_log2.c
SRCS+=		hash_page.c ndbm.c ndbmdatum.c
SRCS+=		bt_close.c bt_conv.c bt_debug.c bt_delete.c bt_get.c bt_open.c
SRCS+=		bt_overflow.c bt_page.c bt_put.c bt_search.c bt_seq.c bt_split.c
SRCS+=		bt_utils.c
SRCS+=		rec_close.c rec_delete.c rec_get.c rec_open.c rec_put.c
SRCS+=		rec_search.c rec_seq.c rec_utils.c
SRCS+=		db.c
SRCS+=		mpool.c
.PATH.c:	/usr/src/lib/libc/db/hash
.PATH.c:	/usr/src/lib/libc/db/btree
.PATH.c:	/usr/src/lib/libc/db/recno
.PATH.c:	/usr/src/lib/libc/db/mpool
.PATH.c:	/usr/src/lib/libc/db/db

# add -DSTATISTICS to CPPFLAGS to get usage statistics.  Note that
# for this to work, libc must be compiled with -DSTATISTICS as well
CPPFLAGS+=	-g -D__DBINTERFACE_PRIVATE
CPPFLAGS+=	-I/usr/src/lib/libc/include
CPPFLAGS+=	-DDEBUG -DSTATISTICS -DHASH_STATISTICS
CPPFLAGS+=	-DDEBUG1 -DDEBUG2 -DDEBUG3 -DDEBUG4 -DDEBUG5 -DDIAGNOSTIC
.endif
CLEANFILES+=	t1 t2 t3

regress: dbtest
	${HOST_SH} ${.CURDIR}/run.test

.include <bsd.prog.mk>
