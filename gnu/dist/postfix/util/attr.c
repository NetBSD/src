/*++
/* NAME
/*	attr 3
/* SUMMARY
/*	attribute list manager
/* SYNOPSIS
/*	#include <htable.h>
/*	#include <attr.h>
/*
/*	void	attr_enter(attr, name, value)
/*	HTABLE	*attr;
/*	const char *name;
/*	const char *value;
/*
/*	const char *attr_find(attr, name)
/*	HTABLE	*attr;
/*	const char *name;
/*
/*	void	attr_free(attr)
/*	HTABLE	*attr;
/* DESCRIPTION
/*	This module maintains open attribute lists of string-valued
/*	names and values. The module is built on top of the generic
/*	htable(3) hash table manager.
/*
/*	attr_enter() adds a new attribute or updates an existing one.
/*	Both the name and the value are copied.
/*
/*	attr_find() looks up the named attribute. It returns the
/*	corresponding value if one is found, a null pointer otherwise.
/*
/*	attr_free() destroys the named attribute list and makes its
/*	memory available for reuse.
/* BUGS
/*	This module cannot store null pointer values. If that is a
/*	problem, use the raw hash table management routines instead.
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

/* System library. */

#include "sys_defs.h"

/* Utility library. */

#include "mymalloc.h"
#include "htable.h"
#include "attr.h"

/* attr_enter - add or replace attribute */

void    attr_enter(HTABLE *attr, const char *name, const char *value)
{
    HTABLE_INFO *ht;

    if ((ht = htable_locate(attr, name)) != 0) {/* replace attribute */
	myfree(ht->value);
	ht->value = mystrdup(value);
    } else {					/* add attribute */
	(void) htable_enter(attr, name, mystrdup(value));
    }
}

/* attr_free - destroy attribute list */

void    attr_free(HTABLE *attr)
{
    htable_free(attr, myfree);
}

