/*	$NetBSD: namespace.h,v 1.2.4.1 1996/09/16 18:40:51 jtc Exp $	*/

#ifndef NAMESPACE_H
#define NAMESPACE_H

#include <sys/cdefs.h>

#ifdef __weak_alias

#define closedir	_closedir
#define opendir		_opendir
#define readdir		_readdir
#define rewinddir	_rewinddir
#define scandir		_scandir
#define seekdir		_seekdir
#define telldir		_telldir
#define alphasort	_alphasort

#define dbopen		_dbopen
#define dbm_open	_dbm_open
#define dbm_close	_dbm_close
#define dbm_fetch	_dbm_fetch
#define dbm_firstkey	_dbm_fetchkey
#define dbm_nextkey	_dbm_nextkey
#define dbm_delete	_dbm_delete
#define dbm_store	_dbm_store
#define dbm_error	_dbm_error
#define dbm_clearerr	_dbm_clearerr 
#define dbm_dirfno	_dbm_dirfno

#define hcreate		_hcreate
#define hsearch		_hsearch
#define hdestroy	_hdestroy
#define mpool_open	_mpool_open
#define mpool_filter	_mpool_filter
#define mpool_new	_mpool_new
#define mpool_get	_mpool_get
#define mpool_put	_mpool_put
#define mpool_close	_mpool_close
#define mpool_sync	_mpool_sync

#define strtoq		_strtoq
#define strtouq		_strtouq
#define catclose	_catclose
#define catgets		_catgets
#define catopen		_catopen
#define sys_errlist	_sys_errlist
#define sys_nerr	_sys_nerr
#define sys_siglist	_sys_siglist
#define err		_err
#define errx		_errx
#define verr		_verr
#define verrx		_verrx
#define vwarn		_vwarn
#define vwarnx		_vwarnx
#define warn		_warn
#define warnx		_warnx

#endif

#endif
