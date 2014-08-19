/*	$NetBSD: dict_alloc.c,v 1.1.1.1.16.2 2014/08/19 23:59:45 tls Exp $	*/

/*++
/* NAME
/*	dict_alloc 3
/* SUMMARY
/*	dictionary memory manager
/* SYNOPSIS
/*	#include <dict.h>
/*
/*	DICT	*dict_alloc(dict_type, dict_name, size)
/*	const char *dict_type;
/*	const char *dict_name;
/*	ssize_t	size;
/*
/*	void	dict_free(dict)
/*	DICT	*ptr;
/*
/*	void	dict_jmp_alloc(dict)
/*	DICT	*ptr;
/* DESCRIPTION
/*	dict_alloc() allocates memory for a dictionary structure of
/*	\fIsize\fR bytes, initializes all generic dictionary
/*	properties to default settings,
/*	and installs default methods that do not support any operation.
/*	The caller is supposed to override the default methods with
/*	ones that it supports.
/*	The purpose of the default methods is to trap an attempt to
/*	invoke an unsupported method.
/*
/*	One exception is the default lock function.  When the
/*	dictionary provides a file handle for locking, the default
/*	lock function returns the result from myflock with the
/*	locking method specified in the lock_type member, otherwise
/*	it returns 0. Presently, the lock function is used only to
/*	implement the DICT_FLAG_OPEN_LOCK feature (lock the database
/*	exclusively after it is opened) for databases that are not
/*	multi-writer safe.
/*
/*	dict_free() releases memory and cleans up after dict_alloc().
/*	It is up to the caller to dispose of any memory that was allocated
/*	by the caller.
/*
/*	dict_jmp_alloc() implements preliminary support for exception
/*	handling. This will eventually be built into dict_alloc().
/*
/*	Arguments:
/* .IP dict_type
/*	The official name for this type of dictionary, as used by
/*	dict_open(3) etc. This is stored under the \fBtype\fR
/*	member.
/* .IP dict_name
/*	Dictionary name. This is stored as the \fBname\fR member.
/* .IP size
/*	The size in bytes of the dictionary subclass structure instance.
/* SEE ALSO
/*	dict(3)
/* DIAGNOSTICS
/*	Fatal errors: the process invokes a default method.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System libraries. */

#include "sys_defs.h"

/* Utility library. */

#include "msg.h"
#include "mymalloc.h"
#include "myflock.h"
#include "dict.h"

/* dict_default_lookup - trap unimplemented operation */

static const char *dict_default_lookup(DICT *dict, const char *unused_key)
{
    msg_fatal("table %s:%s: lookup operation is not supported",
	      dict->type, dict->name);
}

/* dict_default_update - trap unimplemented operation */

static int dict_default_update(DICT *dict, const char *unused_key,
			               const char *unused_value)
{
    msg_fatal("table %s:%s: update operation is not supported",
	      dict->type, dict->name);
}

/* dict_default_delete - trap unimplemented operation */

static int dict_default_delete(DICT *dict, const char *unused_key)
{
    msg_fatal("table %s:%s: delete operation is not supported",
	      dict->type, dict->name);
}

/* dict_default_sequence - trap unimplemented operation */

static int dict_default_sequence(DICT *dict, int unused_function,
		         const char **unused_key, const char **unused_value)
{
    msg_fatal("table %s:%s: sequence operation is not supported",
	      dict->type, dict->name);
}

/* dict_default_lock - default lock handler */

static int dict_default_lock(DICT *dict, int operation)
{
    if (dict->lock_fd >= 0) {
	return (myflock(dict->lock_fd, dict->lock_type, operation));
    } else {
	return (0);
    }
}

/* dict_default_close - trap unimplemented operation */

static void dict_default_close(DICT *dict)
{
    msg_fatal("table %s:%s: close operation is not supported",
	      dict->type, dict->name);
}

/* dict_alloc - allocate dictionary object, initialize super-class */

DICT   *dict_alloc(const char *dict_type, const char *dict_name, ssize_t size)
{
    DICT   *dict = (DICT *) mymalloc(size);

    dict->type = mystrdup(dict_type);
    dict->name = mystrdup(dict_name);
    dict->flags = DICT_FLAG_FIXED;
    dict->lookup = dict_default_lookup;
    dict->update = dict_default_update;
    dict->delete = dict_default_delete;
    dict->sequence = dict_default_sequence;
    dict->close = dict_default_close;
    dict->lock = dict_default_lock;
    dict->lock_type = INTERNAL_LOCK;
    dict->lock_fd = -1;
    dict->stat_fd = -1;
    dict->mtime = 0;
    dict->fold_buf = 0;
    dict->owner.status = DICT_OWNER_UNKNOWN;
    dict->owner.uid = ~0;
    dict->error = DICT_ERR_NONE;
    dict->jbuf = 0;
    return dict;
}

/* dict_free - super-class destructor */

void    dict_free(DICT *dict)
{
    myfree(dict->type);
    myfree(dict->name);
    if (dict->jbuf)
	myfree((char *) dict->jbuf);
    myfree((char *) dict);
}

 /*
  * TODO: add a dict_flags() argument to dict_alloc() and handle jump buffer
  * allocation there.
  */

/* dict_jmp_alloc - enable exception handling */

void    dict_jmp_alloc(DICT *dict)
{
    if (dict->jbuf == 0)
	dict->jbuf = (DICT_JMP_BUF *) mymalloc(sizeof(DICT_JMP_BUF));
}
