#	$NetBSD: Makefile.inc,v 1.7.10.1 2001/03/13 21:20:34 he Exp $
#       @(#)Makefile.inc	8.1 (Berkeley) 6/4/93

.PATH: ${.CURDIR}/db/man

# mpool.3
MAN+=	btree.3 dbopen.3 hash.3 recno.3 mpool.3
MLINKS+= dbopen.3 db.3
MLINKS+= mpool.3 mpool_open.3 mpool.3 mpool_filter.3 mpool.3 mpool_new.3
MLINKS+= mpool.3 mpool_get.3 mpool.3 mpool_put.3 mpool.3 mpool_sync.3
MLINKS+= mpool.3 mpool_close.3
