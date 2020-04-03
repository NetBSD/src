/* Miscellaneous utilities.
   Copyright (C) 2019-2020 Free Software Foundation, Inc.

   This file is part of libctf.

   libctf is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3, or (at your option) any later
   version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not see
   <http://www.gnu.org/licenses/>.  */

#include <ctf-impl.h>
#include <string.h>

/* Simple doubly-linked list append routine.  This implementation assumes that
   each list element contains an embedded ctf_list_t as the first member.
   An additional ctf_list_t is used to store the head (l_next) and tail
   (l_prev) pointers.  The current head and tail list elements have their
   previous and next pointers set to NULL, respectively.  */

void
ctf_list_append (ctf_list_t *lp, void *newp)
{
  ctf_list_t *p = lp->l_prev;	/* p = tail list element.  */
  ctf_list_t *q = newp;		/* q = new list element.  */

  lp->l_prev = q;
  q->l_prev = p;
  q->l_next = NULL;

  if (p != NULL)
    p->l_next = q;
  else
    lp->l_next = q;
}

/* Prepend the specified existing element to the given ctf_list_t.  The
   existing pointer should be pointing at a struct with embedded ctf_list_t.  */

void
ctf_list_prepend (ctf_list_t * lp, void *newp)
{
  ctf_list_t *p = newp;		/* p = new list element.  */
  ctf_list_t *q = lp->l_next;	/* q = head list element.  */

  lp->l_next = p;
  p->l_prev = NULL;
  p->l_next = q;

  if (q != NULL)
    q->l_prev = p;
  else
    lp->l_prev = p;
}

/* Delete the specified existing element from the given ctf_list_t.  The
   existing pointer should be pointing at a struct with embedded ctf_list_t.  */

void
ctf_list_delete (ctf_list_t *lp, void *existing)
{
  ctf_list_t *p = existing;

  if (p->l_prev != NULL)
    p->l_prev->l_next = p->l_next;
  else
    lp->l_next = p->l_next;

  if (p->l_next != NULL)
    p->l_next->l_prev = p->l_prev;
  else
    lp->l_prev = p->l_prev;
}

/* Return 1 if the list is empty.  */

int
ctf_list_empty_p (ctf_list_t *lp)
{
  return (lp->l_next == NULL && lp->l_prev == NULL);
}

/* Convert a 32-bit ELF symbol into Elf64 and return a pointer to it.  */

Elf64_Sym *
ctf_sym_to_elf64 (const Elf32_Sym *src, Elf64_Sym *dst)
{
  dst->st_name = src->st_name;
  dst->st_value = src->st_value;
  dst->st_size = src->st_size;
  dst->st_info = src->st_info;
  dst->st_other = src->st_other;
  dst->st_shndx = src->st_shndx;

  return dst;
}

/* A string appender working on dynamic strings.  Returns NULL on OOM.  */

char *
ctf_str_append (char *s, const char *append)
{
  size_t s_len = 0;

  if (append == NULL)
    return s;

  if (s != NULL)
    s_len = strlen (s);

  size_t append_len = strlen (append);

  if ((s = realloc (s, s_len + append_len + 1)) == NULL)
    return NULL;

  memcpy (s + s_len, append, append_len);
  s[s_len + append_len] = '\0';

  return s;
}

/* A version of ctf_str_append that returns the old string on OOM.  */

char *
ctf_str_append_noerr (char *s, const char *append)
{
  char *new_s;

  new_s = ctf_str_append (s, append);
  if (!new_s)
    return s;
  return new_s;
}

/* A realloc() that fails noisily if called with any ctf_str_num_users.  */
void *
ctf_realloc (ctf_file_t *fp, void *ptr, size_t size)
{
  if (fp->ctf_str_num_refs > 0)
    {
      ctf_dprintf ("%p: attempt to realloc() string table with %lu active refs\n",
		   (void *) fp, (unsigned long) fp->ctf_str_num_refs);
      return NULL;
    }
  return realloc (ptr, size);
}

/* Store the specified error code into errp if it is non-NULL, and then
   return NULL for the benefit of the caller.  */

void *
ctf_set_open_errno (int *errp, int error)
{
  if (errp != NULL)
    *errp = error;
  return NULL;
}

/* Store the specified error code into the CTF container, and then return
   CTF_ERR / -1 for the benefit of the caller. */

unsigned long
ctf_set_errno (ctf_file_t * fp, int err)
{
  fp->ctf_errno = err;
  return CTF_ERR;
}
