/* ELF strtab with GC and suffix merging support.
   Copyright (C) 2001-2020 Free Software Foundation, Inc.
   Written by Jakub Jelinek <jakub@redhat.com>.

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "elf-bfd.h"
#include "hashtab.h"
#include "libiberty.h"

/* An entry in the strtab hash table.  */

struct elf_strtab_hash_entry
{
  struct bfd_hash_entry root;
  /* Length of this entry.  This includes the zero terminator.  */
  int len;
  unsigned int refcount;
  union {
    /* Index within the merged section.  */
    bfd_size_type index;
    /* Entry this is a suffix of (if len < 0).  */
    struct elf_strtab_hash_entry *suffix;
  } u;
};

/* The strtab hash table.  */

struct elf_strtab_hash
{
  struct bfd_hash_table table;
  /* Next available index.  */
  size_t size;
  /* Number of array entries alloced.  */
  size_t alloced;
  /* Final strtab size.  */
  bfd_size_type sec_size;
  /* Array of pointers to strtab entries.  */
  struct elf_strtab_hash_entry **array;
};

/* Routine to create an entry in a section merge hashtab.  */

static struct bfd_hash_entry *
elf_strtab_hash_newfunc (struct bfd_hash_entry *entry,
			 struct bfd_hash_table *table,
			 const char *string)
{
  /* Allocate the structure if it has not already been allocated by a
     subclass.  */
  if (entry == NULL)
    entry = (struct bfd_hash_entry *)
	bfd_hash_allocate (table, sizeof (struct elf_strtab_hash_entry));
  if (entry == NULL)
    return NULL;

  /* Call the allocation method of the superclass.  */
  entry = bfd_hash_newfunc (entry, table, string);

  if (entry)
    {
      /* Initialize the local fields.  */
      struct elf_strtab_hash_entry *ret;

      ret = (struct elf_strtab_hash_entry *) entry;
      ret->u.index = -1;
      ret->refcount = 0;
      ret->len = 0;
    }

  return entry;
}

/* Create a new hash table.  */

struct elf_strtab_hash *
_bfd_elf_strtab_init (void)
{
  struct elf_strtab_hash *table;
  size_t amt = sizeof (struct elf_strtab_hash);

  table = (struct elf_strtab_hash *) bfd_malloc (amt);
  if (table == NULL)
    return NULL;

  if (!bfd_hash_table_init (&table->table, elf_strtab_hash_newfunc,
			    sizeof (struct elf_strtab_hash_entry)))
    {
      free (table);
      return NULL;
    }

  table->sec_size = 0;
  table->size = 1;
  table->alloced = 64;
  amt = sizeof (struct elf_strtab_hasn_entry *);
  table->array = ((struct elf_strtab_hash_entry **)
		  bfd_malloc (table->alloced * amt));
  if (table->array == NULL)
    {
      free (table);
      return NULL;
    }

  table->array[0] = NULL;

  return table;
}

/* Free a strtab.  */

void
_bfd_elf_strtab_free (struct elf_strtab_hash *tab)
{
  bfd_hash_table_free (&tab->table);
  free (tab->array);
  free (tab);
}

/* Get the index of an entity in a hash table, adding it if it is not
   already present.  */

size_t
_bfd_elf_strtab_add (struct elf_strtab_hash *tab,
		     const char *str,
		     bfd_boolean copy)
{
  register struct elf_strtab_hash_entry *entry;

  /* We handle this specially, since we don't want to do refcounting
     on it.  */
  if (*str == '\0')
    return 0;

  BFD_ASSERT (tab->sec_size == 0);
  entry = (struct elf_strtab_hash_entry *)
	  bfd_hash_lookup (&tab->table, str, TRUE, copy);

  if (entry == NULL)
    return (size_t) -1;

  entry->refcount++;
  if (entry->len == 0)
    {
      entry->len = strlen (str) + 1;
      /* 2G strings lose.  */
      BFD_ASSERT (entry->len > 0);
      if (tab->size == tab->alloced)
	{
	  bfd_size_type amt = sizeof (struct elf_strtab_hash_entry *);
	  tab->alloced *= 2;
	  tab->array = (struct elf_strtab_hash_entry **)
	      bfd_realloc_or_free (tab->array, tab->alloced * amt);
	  if (tab->array == NULL)
	    return (size_t) -1;
	}

      entry->u.index = tab->size++;
      tab->array[entry->u.index] = entry;
    }
  return entry->u.index;
}

void
_bfd_elf_strtab_addref (struct elf_strtab_hash *tab, size_t idx)
{
  if (idx == 0 || idx == (size_t) -1)
    return;
  BFD_ASSERT (tab->sec_size == 0);
  BFD_ASSERT (idx < tab->size);
  ++tab->array[idx]->refcount;
}

void
_bfd_elf_strtab_delref (struct elf_strtab_hash *tab, size_t idx)
{
  if (idx == 0 || idx == (size_t) -1)
    return;
  BFD_ASSERT (tab->sec_size == 0);
  BFD_ASSERT (idx < tab->size);
  BFD_ASSERT (tab->array[idx]->refcount > 0);
  --tab->array[idx]->refcount;
}

unsigned int
_bfd_elf_strtab_refcount (struct elf_strtab_hash *tab, size_t idx)
{
  return tab->array[idx]->refcount;
}

void
_bfd_elf_strtab_clear_all_refs (struct elf_strtab_hash *tab)
{
  size_t idx;

  for (idx = 1; idx < tab->size; idx++)
    tab->array[idx]->refcount = 0;
}

/* Save strtab refcounts prior to adding --as-needed library.  */

struct strtab_save
{
  size_t size;
  unsigned int refcount[1];
};

void *
_bfd_elf_strtab_save (struct elf_strtab_hash *tab)
{
  struct strtab_save *save;
  size_t idx, size;

  size = sizeof (*save) + (tab->size - 1) * sizeof (save->refcount[0]);
  save = bfd_malloc (size);
  if (save == NULL)
    return save;

  save->size = tab->size;
  for (idx = 1; idx < tab->size; idx++)
    save->refcount[idx] = tab->array[idx]->refcount;
  return save;
}

/* Restore strtab refcounts on finding --as-needed library not needed.  */

void
_bfd_elf_strtab_restore (struct elf_strtab_hash *tab, void *buf)
{
  size_t idx, curr_size = tab->size, save_size;
  struct strtab_save *save = (struct strtab_save *) buf;

  BFD_ASSERT (tab->sec_size == 0);
  save_size = 1;
  if (save != NULL)
    save_size = save->size;
  BFD_ASSERT (save_size <= curr_size);
  tab->size = save_size;
  for (idx = 1; idx < save_size; ++idx)
    tab->array[idx]->refcount = save->refcount[idx];

  for (; idx < curr_size; ++idx)
    {
      /* We don't remove entries from the hash table, just set their
	 REFCOUNT to zero.  Setting LEN zero will result in the size
	 growing if the entry is added again.  See _bfd_elf_strtab_add.  */
      tab->array[idx]->refcount = 0;
      tab->array[idx]->len = 0;
    }
}

bfd_size_type
_bfd_elf_strtab_size (struct elf_strtab_hash *tab)
{
  return tab->sec_size ? tab->sec_size : tab->size;
}

bfd_size_type
_bfd_elf_strtab_len (struct elf_strtab_hash *tab)
{
  return tab->size;
}

bfd_size_type
_bfd_elf_strtab_offset (struct elf_strtab_hash *tab, size_t idx)
{
  struct elf_strtab_hash_entry *entry;

  if (idx == 0)
    return 0;
  BFD_ASSERT (idx < tab->size);
  BFD_ASSERT (tab->sec_size);
  entry = tab->array[idx];
  BFD_ASSERT (entry->refcount > 0);
  entry->refcount--;
  return tab->array[idx]->u.index;
}

const char *
_bfd_elf_strtab_str (struct elf_strtab_hash *tab, size_t idx,
		     bfd_size_type *offset)
{
  if (idx == 0)
    return 0;
  BFD_ASSERT (idx < tab->size);
  BFD_ASSERT (tab->sec_size);
  if (offset)
    *offset = tab->array[idx]->u.index;
  return tab->array[idx]->root.string;
}

bfd_boolean
_bfd_elf_strtab_emit (register bfd *abfd, struct elf_strtab_hash *tab)
{
  bfd_size_type off = 1;
  size_t i;

  if (bfd_bwrite ("", 1, abfd) != 1)
    return FALSE;

  for (i = 1; i < tab->size; ++i)
    {
      register const char *str;
      register unsigned int len;

      BFD_ASSERT (tab->array[i]->refcount == 0);
      len = tab->array[i]->len;
      if ((int) len < 0)
	continue;

      str = tab->array[i]->root.string;
      if (bfd_bwrite (str, len, abfd) != len)
	return FALSE;

      off += len;
    }

  BFD_ASSERT (off == tab->sec_size);
  return TRUE;
}

/* Compare two elf_strtab_hash_entry structures.  Called via qsort.
   Won't ever return zero as all entries differ, so there is no issue
   with qsort stability here.  */

static int
strrevcmp (const void *a, const void *b)
{
  struct elf_strtab_hash_entry *A = *(struct elf_strtab_hash_entry **) a;
  struct elf_strtab_hash_entry *B = *(struct elf_strtab_hash_entry **) b;
  unsigned int lenA = A->len;
  unsigned int lenB = B->len;
  const unsigned char *s = (const unsigned char *) A->root.string + lenA - 1;
  const unsigned char *t = (const unsigned char *) B->root.string + lenB - 1;
  int l = lenA < lenB ? lenA : lenB;

  while (l)
    {
      if (*s != *t)
	return (int) *s - (int) *t;
      s--;
      t--;
      l--;
    }
  return lenA - lenB;
}

static inline int
is_suffix (const struct elf_strtab_hash_entry *A,
	   const struct elf_strtab_hash_entry *B)
{
  if (A->len <= B->len)
    /* B cannot be a suffix of A unless A is equal to B, which is guaranteed
       not to be equal by the hash table.  */
    return 0;

  return memcmp (A->root.string + (A->len - B->len),
		 B->root.string, B->len - 1) == 0;
}

/* This function assigns final string table offsets for used strings,
   merging strings matching suffixes of longer strings if possible.  */

void
_bfd_elf_strtab_finalize (struct elf_strtab_hash *tab)
{
  struct elf_strtab_hash_entry **array, **a, *e;
  bfd_size_type amt, sec_size;
  size_t size, i;

  /* Sort the strings by suffix and length.  */
  amt = tab->size;
  amt *= sizeof (struct elf_strtab_hash_entry *);
  array = (struct elf_strtab_hash_entry **) bfd_malloc (amt);
  if (array == NULL)
    goto alloc_failure;

  for (i = 1, a = array; i < tab->size; ++i)
    {
      e = tab->array[i];
      if (e->refcount)
	{
	  *a++ = e;
	  /* Adjust the length to not include the zero terminator.  */
	  e->len -= 1;
	}
      else
	e->len = 0;
    }

  size = a - array;
  if (size != 0)
    {
      qsort (array, size, sizeof (struct elf_strtab_hash_entry *), strrevcmp);

      /* Loop over the sorted array and merge suffixes.  Start from the
	 end because we want eg.

	 s1 -> "d"
	 s2 -> "bcd"
	 s3 -> "abcd"

	 to end up as

	 s3 -> "abcd"
	 s2 _____^
	 s1 _______^

	 ie. we don't want s1 pointing into the old s2.  */
      e = *--a;
      e->len += 1;
      while (--a >= array)
	{
	  struct elf_strtab_hash_entry *cmp = *a;

	  cmp->len += 1;
	  if (is_suffix (e, cmp))
	    {
	      cmp->u.suffix = e;
	      cmp->len = -cmp->len;
	    }
	  else
	    e = cmp;
	}
    }

 alloc_failure:
  free (array);

  /* Assign positions to the strings we want to keep.  */
  sec_size = 1;
  for (i = 1; i < tab->size; ++i)
    {
      e = tab->array[i];
      if (e->refcount && e->len > 0)
	{
	  e->u.index = sec_size;
	  sec_size += e->len;
	}
    }

  tab->sec_size = sec_size;

  /* Adjust the rest.  */
  for (i = 1; i < tab->size; ++i)
    {
      e = tab->array[i];
      if (e->refcount && e->len < 0)
	e->u.index = e->u.suffix->u.index + (e->u.suffix->len + e->len);
    }
}
