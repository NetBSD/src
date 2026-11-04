#	$NetBSD: Makefile.inc,v 1.10.104.1 2026/11/04 23:19:16 perseant Exp $
#       @(#)Makefile.inc	8.1 (Berkeley) 6/4/93

.PATH: ${.PARSEDIR}

MAN+=	btree.3 dbm_clearerr.3 dbopen.3 hash.3 recno.3 mpool.3
MLINKS+= dbm_clearerr.3 dbm_close.3 dbm_clearerr.3 dbm_delete.3
MLINKS+= dbm_clearerr.3 dbm_dirfno.3 dbm_clearerr.3 dbm_error.3
MLINKS+= dbm_clearerr.3 dbm_fetch.3 dbm_clearerr.3 dbm_firstkey.3
MLINKS+= dbm_clearerr.3 dbm_nextkey.3 dbm_clearerr.3 dbm_open.3
MLINKS+= dbm_clearerr.3 dbm_store.3 dbm_clearerr.3 ndbm.3
MLINKS+= dbopen.3 db.3
MLINKS+= mpool.3 mpool_open.3 mpool.3 mpool_filter.3 mpool.3 mpool_new.3
MLINKS+= mpool.3 mpool_get.3 mpool.3 mpool_put.3 mpool.3 mpool_sync.3
MLINKS+= mpool.3 mpool_close.3
