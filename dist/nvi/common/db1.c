/*-
 * Copyright (c) 1992, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1992, 1993, 1994, 1995, 1996
 *	Keith Bostic.  All rights reserved.
 *
 * See the LICENSE file for redistribution information.
 */

#include "config.h"

#ifndef lint
static const char sccsid[] = "$Id: db1.c,v 1.1.1.1 2008/05/16 18:03:14 aymeric Exp $ (Berkeley) $Date: 2008/05/16 18:03:14 $";
#endif /* not lint */

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/time.h>

#include <bitstring.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "../vi/vi.h"

static int scr_update __P((SCR *, db_recno_t, lnop_t, int));

/*
 * db_eget --
 *	Front-end to db_get, special case handling for empty files.
 *
 * PUBLIC: int db_eget __P((SCR *, db_recno_t, CHAR_T **, size_t *, int *));
 */
int
db_eget(SCR *sp, db_recno_t lno, CHAR_T **pp, size_t *lenp, int *isemptyp)
	        
	               				/* Line number. */
	            				/* Pointer store. */
	             				/* Length store. */
	              
{
	db_recno_t l1;

	if (isemptyp != NULL)
		*isemptyp = 0;

	/* If the line exists, simply return it. */
	if (!db_get(sp, lno, 0, pp, lenp))
		return (0);

	/*
	 * If the user asked for line 0 or line 1, i.e. the only possible
	 * line in an empty file, find the last line of the file; db_last
	 * fails loudly.
	 */
	if ((lno == 0 || lno == 1) && db_last(sp, &l1))
		return (1);

	/* If the file isn't empty, fail loudly. */
	if ((lno != 0 && lno != 1) || l1 != 0) {
		db_err(sp, lno);
		return (1);
	}

	if (isemptyp != NULL)
		*isemptyp = 1;

	return (1);
}

/*
 * db_get --
 *	Look in the text buffers for a line, followed by the cache, followed
 *	by the database.
 *
 * PUBLIC: int db_get __P((SCR *, db_recno_t, u_int32_t, CHAR_T **, size_t *));
 */
int
db_get(SCR *sp, db_recno_t lno, u_int32_t flags, CHAR_T **pp, size_t *lenp)
	        
	               				/* Line number. */
	                
	            				/* Pointer store. */
	             				/* Length store. */
{
	DBT data, key;
	EXF *ep;
	TEXT *tp;
	db_recno_t l1, l2;
	CHAR_T *wp;
	size_t wlen;
	size_t nlen;

	/*
	 * The underlying recno stuff handles zero by returning NULL, but
	 * have to have an OOB condition for the look-aside into the input
	 * buffer anyway.
	 */
	if (lno == 0)
		goto err1;

	/* Check for no underlying file. */
	if ((ep = sp->ep) == NULL) {
		ex_emsg(sp, NULL, EXM_NOFILEYET);
		goto err3;
	}

	if (LF_ISSET(DBG_NOCACHE))
		goto nocache;

	/*
	 * Look-aside into the TEXT buffers and see if the line we want
	 * is there.
	 */
	if (F_ISSET(sp, SC_TINPUT)) {
		l1 = ((TEXT *)sp->tiq.cqh_first)->lno;
		l2 = ((TEXT *)sp->tiq.cqh_last)->lno;
		if (l1 <= lno && l2 >= lno) {
#if defined(DEBUG) && 0
			vtrace(sp,
			    "retrieve TEXT buffer line %lu\n", (u_long)lno);
#endif
			for (tp = sp->tiq.cqh_first;
			    tp->lno != lno; tp = tp->q.cqe_next);
			if (lenp != NULL)
				*lenp = tp->len;
			if (pp != NULL)
				*pp = tp->lb;
			return (0);
		}
		/*
		 * Adjust the line number for the number of lines used
		 * by the text input buffers.
		 */
		if (lno > l2)
			lno -= l2 - l1;
	}

	/* Look-aside into the cache, and see if the line we want is there. */
	/*
	 * Line cache will not work if different screens view the same
	 * file with different encodings.
	 * Multiple threads accessing the same cache can be a problem as
	 * well.
	 * So, line cache is (temporarily) disabled.
	 */
	if (0 && lno == ep->c_lno) {
#if defined(DEBUG) && 0
		vtrace(sp, "retrieve cached line %lu\n", (u_long)lno);
#endif
		if (lenp != NULL)
			*lenp = ep->c_len;
		if (pp != NULL)
			*pp = ep->c_lp;
		return (0);
	}
	ep->c_lno = OOBLNO;

nocache:
	nlen = 1024;
retry:
	/* data.size contains length in bytes */
	BINC_GOTO(sp, CHAR_T, ep->c_lp, ep->c_blen, nlen);

	/* Get the line from the underlying database. */
	key.data = &lno;
	key.size = sizeof(lno);
	switch (ep->db->actual_db->get(ep->db->actual_db, &key, &data, 0)) {
        case -1:
		goto err2;
	case 1:
err1:		if (LF_ISSET(DBG_FATAL))
err2:			db_err(sp, lno);
alloc_err:
err3:		if (lenp != NULL)
			*lenp = 0;
		if (pp != NULL)
			*pp = NULL;
		return (1);
	case 0:
		if (data.size > nlen) {
			nlen = data.size;
			goto retry;
		} else
			memcpy(ep->c_lp, data.data, nlen);
	}

	if (FILE2INT(sp, data.data, data.size, wp, wlen)) {
	    if (!F_ISSET(sp, SC_CONV_ERROR)) {
		F_SET(sp, SC_CONV_ERROR);
		msgq(sp, M_ERR, "324|Conversion error on line %d", lno);
	    }
	    goto err3;
	}

	/* Reset the cache. */
	if (wp != data.data) {
	    BINC_GOTOW(sp, ep->c_lp, ep->c_blen, wlen);
	    MEMCPYW(ep->c_lp, wp, wlen);
	}
	ep->c_lno = lno;
	ep->c_len = wlen;

#if defined(DEBUG) && 0
	vtrace(sp, "retrieve DB line %lu\n", (u_long)lno);
#endif
	if (lenp != NULL)
		*lenp = wlen;
	if (pp != NULL)
		*pp = ep->c_lp;
	return (0);
}

/*
 * db_delete --
 *	Delete a line from the file.
 *
 * PUBLIC: int db_delete __P((SCR *, db_recno_t));
 */
int
db_delete(SCR *sp, db_recno_t lno)
{
	DBT key;
	EXF *ep;

#if defined(DEBUG) && 0
	vtrace(sp, "delete line %lu\n", (u_long)lno);
#endif
	/* Check for no underlying file. */
	if ((ep = sp->ep) == NULL) {
		ex_emsg(sp, NULL, EXM_NOFILEYET);
		return (1);
	}
	if (ep->l_win && ep->l_win != sp->wp) {
		ex_emsg(sp, NULL, EXM_LOCKED);
		return 1;
	}
		
	/* Update marks, @ and global commands. */
	if (mark_insdel(sp, LINE_DELETE, lno))
		return (1);
	if (ex_g_insdel(sp, LINE_DELETE, lno))
		return (1);

	/* Log change. */
	log_line(sp, lno, LOG_LINE_DELETE);

	/* Update file. */
	key.data = &lno;
	key.size = sizeof(lno);
	sp->db_error = ep->db->actual_db->del(ep->db->actual_db, &key, 0);
	if (sp->db_error != 0) {
		if (sp->db_error == -1)
			sp->db_error = errno;

		msgq(sp, M_DBERR, "003|unable to delete line %lu", 
		    (u_long)lno);
		return (1);
	}

	/* Flush the cache, update line count, before screen update. */
	if (lno <= ep->c_lno)
		ep->c_lno = OOBLNO;
	if (ep->c_nlines != OOBLNO)
		--ep->c_nlines;

	/* File now modified. */
	if (F_ISSET(ep, F_FIRSTMODIFY))
		(void)rcv_init(sp);
	F_SET(ep, F_MODIFIED);

	/* Update screen. */
	return (scr_update(sp, lno, LINE_DELETE, 1));
}

/*
 * db_append --
 *	Append a line into the file.
 *
 * PUBLIC: int db_append __P((SCR *, int, db_recno_t, CHAR_T *, size_t));
 */
int
db_append(SCR *sp, int update, db_recno_t lno, CHAR_T *p, size_t len)
{
	DBT data, key;
	EXF *ep;
	int rval;

#if defined(DEBUG) && 0
	vtrace(sp, "append to %lu: len %u {%.*s}\n", lno, len, MIN(len, 20), p);
#endif
	/* Check for no underlying file. */
	if ((ep = sp->ep) == NULL) {
		ex_emsg(sp, NULL, EXM_NOFILEYET);
		return (1);
	}
	if (ep->l_win && ep->l_win != sp->wp) {
		ex_emsg(sp, NULL, EXM_LOCKED);
		return 1;
	}
		
	/* Update file. */
	key.data = &lno;
	key.size = sizeof(lno);
	data.data = p;
	data.size = len;
	if (ep->db->actual_db->put(ep->db->actual_db, &key, &data, R_IAFTER)) {
		msgq(sp, M_DBERR, "004|unable to append to line %lu", 
			(u_long)lno);
		return (1);
	}

	/* Flush the cache, update line count, before screen update. */
	if (lno < ep->c_lno)
		ep->c_lno = OOBLNO;
	if (ep->c_nlines != OOBLNO)
		++ep->c_nlines;

	/* File now dirty. */
	if (F_ISSET(ep, F_FIRSTMODIFY))
		(void)rcv_init(sp);
	F_SET(ep, F_MODIFIED);

	/* Log change. */
	log_line(sp, lno + 1, LOG_LINE_APPEND);

	/* Update marks, @ and global commands. */
	rval = 0;
	if (mark_insdel(sp, LINE_INSERT, lno + 1))
		rval = 1;
	if (ex_g_insdel(sp, LINE_INSERT, lno + 1))
		rval = 1;

	/*
	 * Update screen.
	 *
	 * XXX
	 * Nasty hack.  If multiple lines are input by the user, they aren't
	 * committed until an <ESC> is entered.  The problem is the screen was
	 * updated/scrolled as each line was entered.  So, when this routine
	 * is called to copy the new lines from the cut buffer into the file,
	 * it has to know not to update the screen again.
	 */
	return (scr_update(sp, lno, LINE_APPEND, update) || rval);
}

/*
 * db_insert --
 *	Insert a line into the file.
 *
 * PUBLIC: int db_insert __P((SCR *, db_recno_t, CHAR_T *, size_t));
 */
int
db_insert(SCR *sp, db_recno_t lno, CHAR_T *p, size_t len)
{
	DBT data, key;
	EXF *ep;
	int rval;

#if defined(DEBUG) && 0
	vtrace(sp, "insert before %lu: len %lu {%.*s}\n",
	    (u_long)lno, (u_long)len, MIN(len, 20), p);
#endif
	/* Check for no underlying file. */
	if ((ep = sp->ep) == NULL) {
		ex_emsg(sp, NULL, EXM_NOFILEYET);
		return (1);
	}
	if (ep->l_win && ep->l_win != sp->wp) {
		ex_emsg(sp, NULL, EXM_LOCKED);
		return 1;
	}
		
	/* Update file. */
	key.data = &lno;
	key.size = sizeof(lno);
	data.data = p;
	data.size = len;
	if (ep->db->actual_db->put(ep->db->actual_db, &key, &data, R_IBEFORE)) {
		msgq(sp, M_SYSERR,
		    "005|unable to insert at line %lu", (u_long)lno);
		return (1);
	}

	/* Flush the cache, update line count, before screen update. */
	if (lno >= ep->c_lno)
		ep->c_lno = OOBLNO;
	if (ep->c_nlines != OOBLNO)
		++ep->c_nlines;

	/* File now dirty. */
	if (F_ISSET(ep, F_FIRSTMODIFY))
		(void)rcv_init(sp);
	F_SET(ep, F_MODIFIED);

	/* Log change. */
	log_line(sp, lno, LOG_LINE_INSERT);

	/* Update marks, @ and global commands. */
	rval = 0;
	if (mark_insdel(sp, LINE_INSERT, lno))
		rval = 1;
	if (ex_g_insdel(sp, LINE_INSERT, lno))
		rval = 1;

	/* Update screen. */
	return (scr_update(sp, lno, LINE_INSERT, 1) || rval);
}

/*
 * db_set --
 *	Store a line in the file.
 *
 * PUBLIC: int db_set __P((SCR *, db_recno_t, CHAR_T *, size_t));
 */
int
db_set(SCR *sp, db_recno_t lno, CHAR_T *p, size_t len)
{
	DBT data, key;
	EXF *ep;
	char *fp;
	size_t flen;

#if defined(DEBUG) && 0
	vtrace(sp, "replace line %lu: len %lu {%.*s}\n",
	    (u_long)lno, (u_long)len, MIN(len, 20), p);
#endif
	/* Check for no underlying file. */
	if ((ep = sp->ep) == NULL) {
		ex_emsg(sp, NULL, EXM_NOFILEYET);
		return (1);
	}
	if (ep->l_win && ep->l_win != sp->wp) {
		ex_emsg(sp, NULL, EXM_LOCKED);
		return 1;
	}
		
	/* Log before change. */
	log_line(sp, lno, LOG_LINE_RESET_B);

	INT2FILE(sp, p, len, fp, flen);

	/* Update file. */
	key.data = &lno;
	key.size = sizeof(lno);
	data.data = fp;
	data.size = flen;
	sp->db_error =
		ep->db->actual_db->put(ep->db->actual_db, &key, &data, 0);
	if (sp->db_error != 0) {
		if (sp->db_error == -1)
			sp->db_error = errno;

		msgq(sp, M_DBERR, "006|unable to store line %lu", (u_long)lno);
		return (1);
	}

	/* Flush the cache, before logging or screen update. */
	if (lno == ep->c_lno)
		ep->c_lno = OOBLNO;

	/* File now dirty. */
	if (F_ISSET(ep, F_FIRSTMODIFY))
		(void)rcv_init(sp);
	F_SET(ep, F_MODIFIED);

	/* Log after change. */
	log_line(sp, lno, LOG_LINE_RESET_F);

	/* Update screen. */
	return (scr_update(sp, lno, LINE_RESET, 1));
}

/*
 * db_exist --
 *	Return if a line exists.
 *
 * PUBLIC: int db_exist __P((SCR *, db_recno_t));
 */
int
db_exist(SCR *sp, db_recno_t lno)
{
	EXF *ep;

	/* Check for no underlying file. */
	if ((ep = sp->ep) == NULL) {
		ex_emsg(sp, NULL, EXM_NOFILEYET);
		return (1);
	}

	if (lno == OOBLNO)
		return (0);
		
	/*
	 * Check the last-line number cache.  Adjust the cached line
	 * number for the lines used by the text input buffers.
	 */
	if (ep->c_nlines != OOBLNO)
		return (lno <= (F_ISSET(sp, SC_TINPUT) ?
		    ep->c_nlines + (((TEXT *)sp->tiq.cqh_last)->lno -
		    ((TEXT *)sp->tiq.cqh_first)->lno) : ep->c_nlines));

	/* Go get the line. */
	return (!db_get(sp, lno, 0, NULL, NULL));
}

/*
 * db_last --
 *	Return the number of lines in the file.
 *
 * PUBLIC: int db_last __P((SCR *, db_recno_t *));
 */
int
db_last(SCR *sp, db_recno_t *lnop)
{
	DBT data, key;
	EXF *ep;
	db_recno_t lno;
	CHAR_T *wp;
	size_t wlen;

	/* Check for no underlying file. */
	if ((ep = sp->ep) == NULL) {
		ex_emsg(sp, NULL, EXM_NOFILEYET);
		return (1);
	}
		
	/*
	 * Check the last-line number cache.  Adjust the cached line
	 * number for the lines used by the text input buffers.
	 */
	if (ep->c_nlines != OOBLNO) {
		*lnop = ep->c_nlines;
		if (F_ISSET(sp, SC_TINPUT))
			*lnop += ((TEXT *)sp->tiq.cqh_last)->lno -
			    ((TEXT *)sp->tiq.cqh_first)->lno;
		return (0);
	}

	key.data = &lno;
	key.size = sizeof(lno);

	sp->db_error = ep->db->actual_db->seq(ep->db->actual_db, &key, &data,
									R_LAST);
	switch (sp->db_error) {
	case 1:
		*lnop = 0;
		return (0);
	case -1:
		sp->db_error = errno;
alloc_err:
		msgq(sp, M_DBERR, "007|unable to get last line");
		*lnop = 0;
		return (1);
        case 0:
		;
	}

	memcpy(&lno, key.data, sizeof(lno));

	if (lno != ep->c_lno) {
	    FILE2INT(sp, data.data, data.size, wp, wlen);

	    /* Fill the cache. */
	    if (wp != data.data) {
		BINC_GOTOW(sp, ep->c_lp, ep->c_blen, wlen);
		MEMCPYW(ep->c_lp, wp, wlen);
	    }
	    ep->c_lno = lno;
	    ep->c_len = wlen;
	}
	ep->c_nlines = lno;

	/* Return the value. */
	*lnop = (F_ISSET(sp, SC_TINPUT) &&
	    ((TEXT *)sp->tiq.cqh_last)->lno > lno ?
	    ((TEXT *)sp->tiq.cqh_last)->lno : lno);
	return (0);
}

/*
 * db_err --
 *	Report a line error.
 *
 * PUBLIC: void db_err __P((SCR *, db_recno_t));
 */
void
db_err(SCR *sp, db_recno_t lno)
{
	msgq(sp, M_ERR,
	    "008|Error: unable to retrieve line %lu", (u_long)lno);
}

/*
 * scr_update --
 *	Update all of the screens that are backed by the file that
 *	just changed.
 */
static int
scr_update(SCR *sp, db_recno_t lno, lnop_t op, int current)
{
	EXF *ep;
	SCR *tsp;
	WIN *wp;

	if (F_ISSET(sp, SC_EX))
		return (0);

	/* XXXX goes outside of window */
	ep = sp->ep;
	if (ep->refcnt != 1)
		for (wp = sp->gp->dq.cqh_first; wp != (void *)&sp->gp->dq; 
		    wp = wp->q.cqe_next)
			for (tsp = wp->scrq.cqh_first;
			    tsp != (void *)&wp->scrq; tsp = tsp->q.cqe_next)
			if (sp != tsp && tsp->ep == ep)
				if (vs_change(tsp, lno, op))
					return (1);
	return (current ? vs_change(sp, lno, op) : 0);
}

/*
 * DB1->3 compatibility layer
 */

#include <assert.h>
#include <stdlib.h>

#undef O_SHLOCK
#undef O_EXLOCK
#include <fcntl.h>

static int db1_close(DB *, u_int32_t);
static int db1_open(DB *, const char *, const char *, DBTYPE, u_int32_t, int);
static int db1_sync(DB *, u_int32_t);
static int db1_get(DB *, DB_TXN *, DBT *, DBT *, u_int32_t);
static int db1_put(DB *, DB_TXN *, DBT *, DBT *, u_int32_t);
static int db1_set_flags(DB *, u_int32_t);
static int db1_set_pagesize(DB *, u_int32_t);
static int db1_set_re_delim(DB *, int);
static int db1_set_re_source(DB *, const char *);

int
db_create(DB **dbp, DB_ENV *dbenv, u_int32_t flags) {
	assert(dbenv == NULL && flags == 0);

	*dbp = malloc(sizeof **dbp);
	if (*dbp == NULL)
		return -1;

	(*dbp)->type = DB_UNKNOWN;
	(*dbp)->actual_db = NULL;
	(*dbp)->_pagesize = 0;
	(*dbp)->_flags = 0;
	memset(&(*dbp)->_recno_info, 0, sizeof (RECNOINFO));

	(*dbp)->close = db1_close;
	(*dbp)->open = db1_open;
	(*dbp)->sync = db1_sync;
	(*dbp)->get = db1_get;
	(*dbp)->put = db1_put;
	(*dbp)->set_flags = db1_set_flags;
	(*dbp)->set_pagesize = db1_set_pagesize;
	(*dbp)->set_re_delim = db1_set_re_delim;
	(*dbp)->set_re_source = db1_set_re_source;

	return 0;
}

char *
db_strerror(int error) {
	return error > 0? strerror(error) : "record not found";
}

static int
db1_close(DB *db, u_int32_t flags) {
	if (flags & DB_NOSYNC) {
		/* XXX warn user? */
	}
	db->actual_db->close(db->actual_db);

	db->type = DB_UNKNOWN;
	db->actual_db = NULL;
	db->_pagesize = 0;
	db->_flags = 0;
	memset(&db->_recno_info, 0, sizeof (RECNOINFO));

	return 0;
}

static int
db1_open(DB *db, const char *file, const char *database, DBTYPE type,
						u_int32_t flags, int mode) {
	int oldflags = 0;

	assert(database == NULL && !(flags & ~(DB_CREATE | DB_TRUNCATE)));

	db->type = type;

	if (flags & DB_CREATE)
		oldflags |= O_CREAT;
	if (flags & DB_TRUNCATE)
		oldflags |= O_TRUNC;

	if (type == DB_RECNO) {
		char *tmp = (char *) file;

		/* The interface is reversed in DB3 */
		file = db->_recno_info.bfname;
		db->_recno_info.bfname = tmp;

		/* ... and so, we should avoid to truncate the main file! */
		oldflags &= ~O_TRUNC;

		db->_recno_info.flags =
			db->_flags & DB_SNAPSHOT? R_SNAPSHOT : 0;
		db->_recno_info.psize = db->_pagesize;
	}

	db->actual_db = dbopen(file, oldflags, mode, type,
				type == DB_RECNO? &db->_recno_info : NULL);

	return db->actual_db == NULL? errno : 0;
}

static int
db1_sync(DB *db, u_int32_t flags) {
	assert(flags == 0);

	return db->actual_db->sync(db->actual_db, db->type == DB_UNKNOWN?
					R_RECNOSYNC : 0) == 0? 0 : errno;
}

static int
db1_get(DB *db, DB_TXN *txnid, DBT *key, DBT *data, u_int32_t flags) {
	int err;

	assert(flags == 0 && txnid == NULL);

	err = db->actual_db->get(db->actual_db, key, data, flags);

	return err == -1? errno : err;
}

static int
db1_put(DB *db, DB_TXN *txnid, DBT *key, DBT *data, u_int32_t flags) {
	int err;

	assert(flags == 0 && txnid == NULL);

	err = db->actual_db->put(db->actual_db, key, data, flags);

	return err == -1? errno : err;
}

static int
db1_set_flags(DB *db, u_int32_t flags) {
	assert((flags & ~(DB_RENUMBER | DB_SNAPSHOT)) == 0);

	/* Can't prevent renumbering from happening with DB1 */
	assert((flags | db->_flags) & DB_RENUMBER);


	db->_flags |= flags;

	return 0;
}

static int
db1_set_pagesize(DB *db, u_int32_t pagesize) {
	db->_pagesize = pagesize;

	return 0;
}

static int
db1_set_re_delim(DB *db, int re_delim) {
	db->_recno_info.bval = re_delim;

	return 0;
}

static int
db1_set_re_source(DB *db, const char *re_source) {
	db->_recno_info.bfname = (char *) re_source;

	return 0;
}
