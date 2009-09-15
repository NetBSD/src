/*	$NetBSD: nvtable.c,v 1.1.1.1.2.2 2009/09/15 06:04:01 snj Exp $	*/

/*++
/* NAME
/*	nvtable 3
/* SUMMARY
/*	attribute list manager
/* SYNOPSIS
/*	#include <nvtable.h>
/*
/*	typedef	struct {
/* .in +4
/*		char	*key;
/*		char	*value;
/*		/* private fields... */
/* .in -4
/*	} NVTABLE_INFO;
/*
/*	NVTABLE	*nvtable_create(size)
/*	int	size;
/*
/*	NVTABLE_INFO *nvtable_update(table, key, value)
/*	NVTABLE	*table;
/*	const char *key;
/*	const char *value;
/*
/*	char	*nvtable_find(table, key)
/*	NVTABLE	*table;
/*	const char *key;
/*
/*	NVTABLE_INFO *nvtable_locate(table, key)
/*	NVTABLE	*table;
/*	const char *key;
/*
/*	void	nvtable_delete(table, key)
/*	NVTABLE	*table;
/*	const char *key;
/*
/*	void	nvtable_free(table)
/*	NVTABLE	*table;
/*
/*	void	nvtable_walk(table, action, ptr)
/*	NVTABLE	*table;
/*	void	(*action)(NVTABLE_INFO *, char *ptr);
/*	char	*ptr;
/*
/*	NVTABLE_INFO **nvtable_list(table)
/*	NVTABLE	*table;
/* DESCRIPTION
/*	This module maintains one or more attribute lists. It provides a
/*	more convenient interface than hash tables, although it uses the
/*	same underlying implementation. Each attribute list entry consists
/*	of a unique string-valued lookup key and a string value.
/*
/*	nvtable_create() creates a table of the specified size and returns a
/*	pointer to the result.
/*
/*	nvtable_update() stores or updates a (key, value) pair in the specified
/*	table and returns a pointer to the resulting entry. The key and the
/*	value are copied.
/*
/*	nvtable_find() returns the value that was stored under the given key,
/*	or a null pointer if it was not found. In order to distinguish
/*	a null value from a non-existent value, use nvtable_locate().
/*
/*	nvtable_locate() returns a pointer to the entry that was stored
/*	for the given key, or a null pointer if it was not found.
/*
/*	nvtable_delete() removes one entry that was stored under the given key.
/*
/*	nvtable_free() destroys a hash table, including contents.
/*
/*	nvtable_walk() invokes the action function for each table entry, with
/*	a pointer to the entry as its argument. The ptr argument is passed
/*	on to the action function.
/*
/*	nvtable_list() returns a null-terminated list of pointers to
/*	all elements in the named table. The list should be passed to
/*	myfree().
/* RESTRICTIONS
/*	A callback function should not modify the attribute list that is
/*	specified to its caller.
/* DIAGNOSTICS
/*	The following conditions are reported and cause the program to
/*	terminate immediately: memory allocation failure; an attempt
/*	to delete a non-existent entry.
/* SEE ALSO
/*	mymalloc(3) memory management wrapper
/*	htable(3) hash table manager
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

/* C library */

#include <sys_defs.h>

/* Utility library. */

#include <mymalloc.h>
#include <htable.h>
#include <nvtable.h>

/* nvtable_update - update or enter (key, value) pair */

NVTABLE_INFO *nvtable_update(NVTABLE * table, const char *key, const char *value)
{
    NVTABLE_INFO *ht;

    if ((ht = htable_locate(table, key)) != 0) {
	myfree(ht->value);
    } else {
	ht = htable_enter(table, key, (char *) 0);
    }
    ht->value = mystrdup(value);
    return (ht);
}
