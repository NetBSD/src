#	$NetBSD: Makefile.inc,v 1.7 1999/03/08 18:43:21 kleink Exp $
#       @(#)Makefile.inc	8.1 (Berkeley) 6/4/93

.PATH: ${.CURDIR}/db/man

# mpool.3
MAN+=	btree.3 dbopen.3 hash.3 hcreate.3 recno.3 mpool.3
MLINKS+= dbopen.3 db.3
MLINKS+= hcreate.3 hdestroy.3
MLINKS+= hcreate.3 hsearch.3
MLINKS+= mpool.3 mpool_open.3 mpool.3 mpool_filter.3 mpool.3 mpool_new.3
MLINKS+= mpool.3 mpool_get.3 mpool.3 mpool_put.3 mpool.3 mpool_sync.3
MLINKS+= mpool.3 mpool_close.3
