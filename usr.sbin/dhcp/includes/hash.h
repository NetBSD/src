/* hash.h

   Definitions for hashing... */

/*
 * Copyright (c) 1996-1999 Internet Software Consortium.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The Internet Software Consortium nor the names
 *    of its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INTERNET SOFTWARE CONSORTIUM AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE INTERNET SOFTWARE CONSORTIUM OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This software has been written for the Internet Software Consortium
 * by Ted Lemon in cooperation with Vixie Enterprises and Nominum, Inc.
 * To learn more about the Internet Software Consortium, see
 * ``http://www.isc.org/''.  To learn more about Vixie Enterprises,
 * see ``http://www.vix.com''.   To learn more about Nominum, Inc., see
 * ``http://www.nominum.com''.
 */

#define DEFAULT_HASH_SIZE	9973

/* The purpose of the hashed_object_t struct is to not match anything else. */
typedef struct {
	int foo;
} hashed_object_t;

typedef void (*hash_foreach_func) (const unsigned char *,
				   unsigned, hashed_object_t *);
typedef int (*hash_reference) (hashed_object_t **, hashed_object_t *,
			       const char *, int);
typedef int (*hash_dereference) (hashed_object_t **, const char *, int);

struct hash_bucket {
	struct hash_bucket *next;
	const unsigned char *name;
	unsigned len;
	hashed_object_t *value;
};

typedef int (*hash_comparator_t)(const void *, const void *, unsigned long);

struct hash_table {
	unsigned hash_count;
	struct hash_bucket *buckets [DEFAULT_HASH_SIZE];
	hash_reference referencer;
	hash_dereference dereferencer;
	hash_comparator_t cmp;
	int (*do_hash) (const unsigned char *, unsigned, unsigned);
};

struct named_hash {
	struct named_hash *next;
	const char *name;
	struct hash_table *hash;
};

#define HASH_FUNCTIONS_DECL(name, bufarg, type)				      \
void name##_hash_add (struct hash_table *, bufarg, unsigned, type *,	      \
		      const char *, int);				      \
void name##_hash_delete (struct hash_table *, bufarg, unsigned,		      \
			 const char *, int);				      \
int name##_hash_lookup (type **, struct hash_table *, bufarg, unsigned,	      \
			const char *, int);				      \
int name##_hash_foreach (struct hash_table *,				      \
			 void (*) (bufarg, unsigned, type *));


#define HASH_FUNCTIONS(name, bufarg, type)				      \
void name##_hash_add (struct hash_table *table,				      \
		      bufarg buf, unsigned len, type *ptr,		      \
		      const char *file, int line)			      \
{									      \
	add_hash (table,						      \
		  (const unsigned char *)buf,				      \
		  len, (hashed_object_t *)ptr, file, line);		      \
}									      \
									      \
void name##_hash_delete (struct hash_table *table,			      \
			 bufarg buf, unsigned len, const char *file, int line)\
{									      \
	delete_hash_entry (table, (const unsigned char *)buf,		      \
			   len, file, line);				      \
}									      \
									      \
int name##_hash_lookup (type **ptr, struct hash_table *table,		      \
			bufarg buf, unsigned len, const char *file, int line) \
{									      \
	return hash_lookup ((hashed_object_t **)ptr, table,		      \
			    (const unsigned char *)buf, len, file, line);     \
}									      \
									      \
int name##_hash_foreach (struct hash_table *table,			      \
			 void (*func) (bufarg, unsigned, type *))	      \
{									      \
	return hash_foreach (table, (hash_foreach_func)func);		      \
}

			    
