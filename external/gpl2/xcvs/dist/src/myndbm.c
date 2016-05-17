/*
 * Copyright (C) 1986-2005 The Free Software Foundation, Inc.
 *
 * Portions Copyright (C) 1998-2005 Derek Price, Ximbiot <http://ximbiot.com>,
 *                                  and others.
 *
 * Portions Copyright (C) 1992, Brian Berliner and Jeff Polk
 * Portions Copyright (C) 1989-1992, Brian Berliner
 * 
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS source distribution.
 * 
 * A simple ndbm-emulator for CVS.  It parses a text file of the format:
 * 
 * key	value
 * 
 * at dbm_open time, and loads the entire file into memory.  As such, it is
 * probably only good for fairly small modules files.  Ours is about 30K in
 * size, and this code works fine.
 */
#include <sys/cdefs.h>
__RCSID("$NetBSD: myndbm.c,v 1.2 2016/05/17 14:00:09 christos Exp $");

#include "cvs.h"

#include "getdelim.h"
#include "getline.h"

#ifdef MY_NDBM
# ifndef O_ACCMODE
#   define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR)
# endif /* defined O_ACCMODE */

static void mydbm_load_file (FILE *, List *, char *);

/* Returns NULL on error in which case errno has been set to indicate
   the error.  Can also call error() itself.  */
/* ARGSUSED */
DBM *
mydbm_open (char *file, int flags, int mode)
{
    FILE *fp;
    DBM *db;

    fp = CVS_FOPEN (file, (flags & O_ACCMODE) != O_RDONLY
			  ?  FOPEN_BINARY_READWRITE : FOPEN_BINARY_READ);
    if (fp == NULL && !(existence_error (errno) && (flags & O_CREAT)))
	return NULL;

    db = xmalloc (sizeof (*db));
    db->dbm_list = getlist ();
    db->modified = 0;
    db->name = xstrdup (file);

    if (fp != NULL)
    {
	mydbm_load_file (fp, db->dbm_list, file);
	if (fclose (fp) < 0)
	    error (0, errno, "cannot close %s",
		   primary_root_inverse_translate (file));
    }
    return db;
}



static int
write_item (Node *node, void *data)
{
    FILE *fp = data;
    fputs (node->key, fp);
    fputs (" ", fp);
    fputs (node->data, fp);
    fputs ("\012", fp);
    return 0;
}



void
mydbm_close (DBM *db)
{
    if (db->modified)
    {
	FILE *fp;
	fp = CVS_FOPEN (db->name, FOPEN_BINARY_WRITE);
	if (fp == NULL)
	    error (1, errno, "cannot write %s", db->name);
	walklist (db->dbm_list, write_item, fp);
	if (fclose (fp) < 0)
	    error (0, errno, "cannot close %s", db->name);
    }
    free (db->name);
    dellist (&db->dbm_list);
    free (db);
}



datum
mydbm_fetch (DBM *db, datum key)
{
    Node *p;
    char *s;
    datum val;

    /* make sure it's null-terminated */
    s = xmalloc (key.dsize + 1);
    (void) strncpy (s, key.dptr, key.dsize);
    s[key.dsize] = '\0';

    p = findnode (db->dbm_list, s);
    if (p)
    {
	val.dptr = p->data;
	val.dsize = strlen (p->data);
    }
    else
    {
	val.dptr = NULL;
	val.dsize = 0;
    }
    free (s);
    return val;
}



datum
mydbm_firstkey (DBM *db)
{
    Node *head, *p;
    datum key;

    head = db->dbm_list->list;
    p = head->next;
    if (p != head)
    {
	key.dptr = p->key;
	key.dsize = strlen (p->key);
    }
    else
    {
	key.dptr = NULL;
	key.dsize = 0;
    }
    db->dbm_next = p->next;
    return key;
}



datum
mydbm_nextkey (DBM *db)
{
    Node *head, *p;
    datum key;

    head = db->dbm_list->list;
    p = db->dbm_next;
    if (p != head)
    {
	key.dptr = p->key;
	key.dsize = strlen (p->key);
    }
    else
    {
	key.dptr = NULL;
	key.dsize = 0;
    }
    db->dbm_next = p->next;
    return key;
}



/* Note: only updates the in-memory copy, which is written out at
   mydbm_close time.  Note: Also differs from DBM in that on duplication,
   it gives a warning, rather than either DBM_INSERT or DBM_REPLACE
   behavior.  */
int
mydbm_store (DBM *db, datum key, datum value, int flags)
{
    Node *node;

    node = getnode ();
    node->type = NDBMNODE;

    node->key = xmalloc (key.dsize + 1);
    *node->key = '\0';
    strncat (node->key, key.dptr, key.dsize);

    node->data = xmalloc (value.dsize + 1);
    *(char *)node->data = '\0';
    strncat (node->data, value.dptr, value.dsize);

    db->modified = 1;
    if (addnode (db->dbm_list, node) == -1)
    {
	error (0, 0, "attempt to insert duplicate key `%s'", node->key);
	freenode (node);
	return 0;
    }
    return 0;
}



/* Load a DBM file.
 *
 * INPUTS
 *   filename		Used in error messages.
 */
static void
mydbm_load_file (FILE *fp, List *list, char *filename)
{
    char *line = NULL;
    size_t line_size;
    char *value;
    size_t value_allocated;
    char *cp, *vp;
    int cont;
    int line_length;
    int line_num;

    value_allocated = 1;
    value = xmalloc (value_allocated);

    cont = 0;
    line_num=0;
    while ((line_length = getdelim (&line, &line_size, '\012', fp)) >= 0)
    {
	line_num++;
	if (line_length > 0 && line[line_length - 1] == '\012')
	{
	    /* Strip the newline.  */
	    --line_length;
	    line[line_length] = '\0';
	}
	if (line_length > 0 && line[line_length - 1] == '\015')
	{
	    /* If the file (e.g. modules) was written on an NT box, it will
	       contain CRLF at the ends of lines.  Strip them (we can't do
	       this by opening the file in text mode because we might be
	       running on unix).  */
	    --line_length;
	    line[line_length] = '\0';
	}

	/*
	 * Add the line to the value, at the end if this is a continuation
	 * line; otherwise at the beginning, but only after any trailing
	 * backslash is removed.
	 */
	if (!cont)
	    value[0] = '\0';

	/*
	 * See if the line we read is a continuation line, and strip the
	 * backslash if so.
	 */
	if (line_length > 0)
	    cp = &line[line_length - 1];
	else
	    cp = line;
	if (*cp == '\\')
	{
	    cont = 1;
	    *cp = '\0';
	    --line_length;
	}
	else
	{
	    cont = 0;
	}
	expand_string (&value,
		       &value_allocated,
		       strlen (value) + line_length + 5);
	strcat (value, line);

	if (value[0] == '#')
	    continue;			/* comment line */
	vp = value;
	while (*vp && isspace ((unsigned char) *vp))
	    vp++;
	if (*vp == '\0')
	    continue;			/* empty line */

	/*
	 * If this was not a continuation line, add the entry to the database
	 */
	if (!cont)
	{
	    Node *p = getnode ();
	    char *kp;

	    kp = vp;
	    while (*vp && !isspace ((unsigned char) *vp))
		vp++;
	    if (*vp)
		*vp++ = '\0';		/* NULL terminate the key */
	    p->type = NDBMNODE;
	    p->key = xstrdup (kp);
	    while (*vp && isspace ((unsigned char) *vp))
		vp++;			/* skip whitespace to value */
	    if (*vp == '\0')
	    {
		if (!really_quiet)
		    error (0, 0,
			"warning: NULL value for key `%s' at line %d of `%s'",
			p->key, line_num,
			primary_root_inverse_translate (filename));
		freenode (p);
		continue;
	    }
	    p->data = xstrdup (vp);
	    if (addnode (list, p) == -1)
	    {
		if (!really_quiet)
		    error (0, 0,
			"duplicate key found for `%s' at line %d of `%s'",
			p->key, line_num,
			primary_root_inverse_translate (filename));
		freenode (p);
	    }
	}
    }
    if (line_length < 0 && !feof (fp))
	error (0, errno, "cannot read file `%s' in mydbm_load_file",
	       primary_root_inverse_translate (filename));

    free (line);
    free (value);
}

#endif				/* MY_NDBM */
