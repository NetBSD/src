/* GNU gettext - internationalization aids
   Copyright (C) 1995-1998, 2000-2004 Free Software Foundation, Inc.

   This file was written by Peter Miller <millerp@canb.auug.org.au>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Specification.  */
#include "message.h"

#include <stdlib.h>
#include <string.h>

#include "fstrcmp.h"
#include "hash.h"
#include "xalloc.h"


const char *const format_language[NFORMATS] =
{
  /* format_c */		"c",
  /* format_objc */		"objc",
  /* format_sh */		"sh",
  /* format_python */		"python",
  /* format_lisp */		"lisp",
  /* format_elisp */		"elisp",
  /* format_librep */		"librep",
  /* format_scheme */		"scheme",
  /* format_smalltalk */	"smalltalk",
  /* format_java */		"java",
  /* format_csharp */		"csharp",
  /* format_awk */		"awk",
  /* format_pascal */		"object-pascal",
  /* format_ycp */		"ycp",
  /* format_tcl */		"tcl",
  /* format_perl */		"perl",
  /* format_perl_brace */	"perl-brace",
  /* format_php */		"php",
  /* format_gcc_internal */	"gcc-internal",
  /* format_qt */		"qt"
};

const char *const format_language_pretty[NFORMATS] =
{
  /* format_c */		"C",
  /* format_objc */		"Objective C",
  /* format_sh */		"Shell",
  /* format_python */		"Python",
  /* format_lisp */		"Lisp",
  /* format_elisp */		"Emacs Lisp",
  /* format_librep */		"librep",
  /* format_scheme */		"Scheme",
  /* format_smalltalk */	"Smalltalk",
  /* format_java */		"Java",
  /* format_csharp */		"C#",
  /* format_awk */		"awk",
  /* format_pascal */		"Object Pascal",
  /* format_ycp */		"YCP",
  /* format_tcl */		"Tcl",
  /* format_perl */		"Perl",
  /* format_perl_brace */	"Perl brace",
  /* format_php */		"PHP",
  /* format_gcc_internal */	"GCC internal",
  /* format_qt */		"Qt"
};


bool
possible_format_p (enum is_format is_format)
{
  return is_format == possible
	 || is_format == yes_according_to_context
	 || is_format == yes;
}


message_ty *
message_alloc (const char *msgid, const char *msgid_plural,
	       const char *msgstr, size_t msgstr_len,
	       const lex_pos_ty *pp)
{
  message_ty *mp;
  size_t i;

  mp = (message_ty *) xmalloc (sizeof (message_ty));
  mp->msgid = msgid;
  mp->msgid_plural = (msgid_plural != NULL ? xstrdup (msgid_plural) : NULL);
  mp->msgstr = msgstr;
  mp->msgstr_len = msgstr_len;
  mp->pos = *pp;
  mp->comment = NULL;
  mp->comment_dot = NULL;
  mp->filepos_count = 0;
  mp->filepos = NULL;
  mp->is_fuzzy = false;
  for (i = 0; i < NFORMATS; i++)
    mp->is_format[i] = undecided;
  mp->do_wrap = undecided;
  mp->used = 0;
  mp->obsolete = false;
  return mp;
}


void
message_free (message_ty *mp)
{
  size_t j;

  free ((char *) mp->msgid);
  if (mp->msgid_plural != NULL)
    free ((char *) mp->msgid_plural);
  free ((char *) mp->msgstr);
  if (mp->comment != NULL)
    string_list_free (mp->comment);
  if (mp->comment_dot != NULL)
    string_list_free (mp->comment_dot);
  for (j = 0; j < mp->filepos_count; ++j)
    free ((char *) mp->filepos[j].file_name);
  if (mp->filepos != NULL)
    free (mp->filepos);
  free (mp);
}


void
message_comment_append (message_ty *mp, const char *s)
{
  if (mp->comment == NULL)
    mp->comment = string_list_alloc ();
  string_list_append (mp->comment, s);
}


void
message_comment_dot_append (message_ty *mp, const char *s)
{
  if (mp->comment_dot == NULL)
    mp->comment_dot = string_list_alloc ();
  string_list_append (mp->comment_dot, s);
}


void
message_comment_filepos (message_ty *mp, const char *name, size_t line)
{
  size_t j;
  size_t nbytes;
  lex_pos_ty *pp;

  /* See if we have this position already.  */
  for (j = 0; j < mp->filepos_count; j++)
    {
      pp = &mp->filepos[j];
      if (strcmp (pp->file_name, name) == 0 && pp->line_number == line)
	return;
    }

  /* Extend the list so that we can add a position to it.  */
  nbytes = (mp->filepos_count + 1) * sizeof (mp->filepos[0]);
  mp->filepos = xrealloc (mp->filepos, nbytes);

  /* Insert the position at the end.  Don't sort the file positions here.  */
  pp = &mp->filepos[mp->filepos_count++];
  pp->file_name = xstrdup (name);
  pp->line_number = line;
}


message_ty *
message_copy (message_ty *mp)
{
  message_ty *result;
  size_t j, i;

  result = message_alloc (xstrdup (mp->msgid), mp->msgid_plural,
			  mp->msgstr, mp->msgstr_len, &mp->pos);

  if (mp->comment)
    {
      for (j = 0; j < mp->comment->nitems; ++j)
	message_comment_append (result, mp->comment->item[j]);
    }
  if (mp->comment_dot)
    {
      for (j = 0; j < mp->comment_dot->nitems; ++j)
	message_comment_dot_append (result, mp->comment_dot->item[j]);
    }
  result->is_fuzzy = mp->is_fuzzy;
  for (i = 0; i < NFORMATS; i++)
    result->is_format[i] = mp->is_format[i];
  result->do_wrap = mp->do_wrap;
  for (j = 0; j < mp->filepos_count; ++j)
    {
      lex_pos_ty *pp = &mp->filepos[j];
      message_comment_filepos (result, pp->file_name, pp->line_number);
    }
  return result;
}


message_list_ty *
message_list_alloc (bool use_hashtable)
{
  message_list_ty *mlp;

  mlp = (message_list_ty *) xmalloc (sizeof (message_list_ty));
  mlp->nitems = 0;
  mlp->nitems_max = 0;
  mlp->item = NULL;
  if ((mlp->use_hashtable = use_hashtable))
    init_hash (&mlp->htable, 10);
  return mlp;
}


void
message_list_free (message_list_ty *mlp)
{
  size_t j;

  for (j = 0; j < mlp->nitems; ++j)
    message_free (mlp->item[j]);
  if (mlp->item)
    free (mlp->item);
  if (mlp->use_hashtable)
    delete_hash (&mlp->htable);
  free (mlp);
}


void
message_list_append (message_list_ty *mlp, message_ty *mp)
{
  if (mlp->nitems >= mlp->nitems_max)
    {
      size_t nbytes;

      mlp->nitems_max = mlp->nitems_max * 2 + 4;
      nbytes = mlp->nitems_max * sizeof (message_ty *);
      mlp->item = xrealloc (mlp->item, nbytes);
    }
  mlp->item[mlp->nitems++] = mp;

  if (mlp->use_hashtable)
    if (insert_entry (&mlp->htable, mp->msgid, strlen (mp->msgid) + 1, mp))
      /* A message list has duplicates, although it was allocated with the
	 assertion that it wouldn't have duplicates.  It is a bug.  */
      abort ();
}


void
message_list_prepend (message_list_ty *mlp, message_ty *mp)
{
  size_t j;

  if (mlp->nitems >= mlp->nitems_max)
    {
      size_t nbytes;

      mlp->nitems_max = mlp->nitems_max * 2 + 4;
      nbytes = mlp->nitems_max * sizeof (message_ty *);
      mlp->item = xrealloc (mlp->item, nbytes);
    }
  for (j = mlp->nitems; j > 0; j--)
    mlp->item[j] = mlp->item[j - 1];
  mlp->item[0] = mp;
  mlp->nitems++;

  if (mlp->use_hashtable)
    if (insert_entry (&mlp->htable, mp->msgid, strlen (mp->msgid) + 1, mp))
      /* A message list has duplicates, although it was allocated with the
	 assertion that it wouldn't have duplicates.  It is a bug.  */
      abort ();
}


void
message_list_insert_at (message_list_ty *mlp, size_t n, message_ty *mp)
{
  size_t j;

  if (mlp->nitems >= mlp->nitems_max)
    {
      size_t nbytes;

      mlp->nitems_max = mlp->nitems_max * 2 + 4;
      nbytes = mlp->nitems_max * sizeof (message_ty *);
      mlp->item = xrealloc (mlp->item, nbytes);
    }
  for (j = mlp->nitems; j > n; j--)
    mlp->item[j] = mlp->item[j - 1];
  mlp->item[j] = mp;
  mlp->nitems++;

  if (mlp->use_hashtable)
    if (insert_entry (&mlp->htable, mp->msgid, strlen (mp->msgid) + 1, mp))
      /* A message list has duplicates, although it was allocated with the
	 assertion that it wouldn't have duplicates.  It is a bug.  */
      abort ();
}


#if 0 /* unused */
void
message_list_delete_nth (message_list_ty *mlp, size_t n)
{
  size_t j;

  if (n >= mlp->nitems)
    return;
  message_free (mlp->item[n]);
  for (j = n + 1; j < mlp->nitems; ++j)
    mlp->item[j - 1] = mlp->item[j];
  mlp->nitems--;

  if (mlp->use_hashtable)
    {
      /* Our simple-minded hash tables don't support removal.  */
      delete_hash (&mlp->htable);
      mlp->use_hashtable = false;
    }
}
#endif


void
message_list_remove_if_not (message_list_ty *mlp,
			    message_predicate_ty *predicate)
{
  size_t i, j;

  for (j = 0, i = 0; j < mlp->nitems; j++)
    if (predicate (mlp->item[j]))
      mlp->item[i++] = mlp->item[j];
  if (mlp->use_hashtable && i < mlp->nitems)
    {
      /* Our simple-minded hash tables don't support removal.  */
      delete_hash (&mlp->htable);
      mlp->use_hashtable = false;
    }
  mlp->nitems = i;
}


bool
message_list_msgids_changed (message_list_ty *mlp)
{
  if (mlp->use_hashtable)
    {
      unsigned long int size = mlp->htable.size;
      size_t j;

      delete_hash (&mlp->htable);
      init_hash (&mlp->htable, size);

      for (j = 0; j < mlp->nitems; j++)
	{
	  message_ty *mp = mlp->item[j];

	  if (insert_entry (&mlp->htable, mp->msgid, strlen (mp->msgid) + 1,
			    mp))
	    /* A message list has duplicates, although it was allocated with
	       the assertion that it wouldn't have duplicates, and before the
	       msgids changed it indeed didn't have duplicates.  */
	    {
	      delete_hash (&mlp->htable);
	      mlp->use_hashtable = false;
	      return true;
	    }
	}
    }
  return false;
}


message_ty *
message_list_search (message_list_ty *mlp, const char *msgid)
{
  if (mlp->use_hashtable)
    {
      void *htable_value;

      if (find_entry (&mlp->htable, msgid, strlen (msgid) + 1, &htable_value))
	return NULL;
      else
	return (message_ty *) htable_value;
    }
  else
    {
      size_t j;

      for (j = 0; j < mlp->nitems; ++j)
	{
	  message_ty *mp;

	  mp = mlp->item[j];
	  if (strcmp (msgid, mp->msgid) == 0)
	    return mp;
	}
      return NULL;
    }
}


static message_ty *
message_list_search_fuzzy_inner (message_list_ty *mlp, const char *msgid,
				 double *best_weight_p)
{
  size_t j;
  message_ty *best_mp;

  best_mp = NULL;
  for (j = 0; j < mlp->nitems; ++j)
    {
      message_ty *mp;

      mp = mlp->item[j];

      if (mp->msgstr != NULL && mp->msgstr[0] != '\0')
	{
	  double weight = fstrcmp (msgid, mp->msgid);
	  if (weight > *best_weight_p)
	    {
	      *best_weight_p = weight;
	      best_mp = mp;
	    }
	}
    }
  return best_mp;
}


message_ty *
message_list_search_fuzzy (message_list_ty *mlp, const char *msgid)
{
  double best_weight;

  best_weight = 0.6;
  return message_list_search_fuzzy_inner (mlp, msgid, &best_weight);
}


message_list_list_ty *
message_list_list_alloc ()
{
  message_list_list_ty *mllp;

  mllp = (message_list_list_ty *) xmalloc (sizeof (message_list_list_ty));
  mllp->nitems = 0;
  mllp->nitems_max = 0;
  mllp->item = NULL;
  return mllp;
}


#if 0 /* unused */
void
message_list_list_free (message_list_list_ty *mllp)
{
  size_t j;

  for (j = 0; j < mllp->nitems; ++j)
    message_list_free (mllp->item[j]);
  if (mllp->item)
    free (mllp->item);
  free (mllp);
}
#endif


void
message_list_list_append (message_list_list_ty *mllp, message_list_ty *mlp)
{
  if (mllp->nitems >= mllp->nitems_max)
    {
      size_t nbytes;

      mllp->nitems_max = mllp->nitems_max * 2 + 4;
      nbytes = mllp->nitems_max * sizeof (message_list_ty *);
      mllp->item = xrealloc (mllp->item, nbytes);
    }
  mllp->item[mllp->nitems++] = mlp;
}


void
message_list_list_append_list (message_list_list_ty *mllp,
			       message_list_list_ty *mllp2)
{
  size_t j;

  for (j = 0; j < mllp2->nitems; ++j)
    message_list_list_append (mllp, mllp2->item[j]);
}


message_ty *
message_list_list_search (message_list_list_ty *mllp, const char *msgid)
{
  message_ty *best_mp;
  int best_weight; /* 0: not found, 1: found without msgstr, 2: translated */
  size_t j;

  best_mp = NULL;
  best_weight = 0;
  for (j = 0; j < mllp->nitems; ++j)
    {
      message_list_ty *mlp;
      message_ty *mp;

      mlp = mllp->item[j];
      mp = message_list_search (mlp, msgid);
      if (mp)
	{
	  int weight = (mp->msgstr_len == 1 && mp->msgstr[0] == '\0' ? 1 : 2);
	  if (weight > best_weight)
	    {
	      best_mp = mp;
	      best_weight = weight;
	    }
	}
    }
  return best_mp;
}


message_ty *
message_list_list_search_fuzzy (message_list_list_ty *mllp, const char *msgid)
{
  size_t j;
  double best_weight;
  message_ty *best_mp;

  best_weight = 0.6;
  best_mp = NULL;
  for (j = 0; j < mllp->nitems; ++j)
    {
      message_list_ty *mlp;
      message_ty *mp;

      mlp = mllp->item[j];
      mp = message_list_search_fuzzy_inner (mlp, msgid, &best_weight);
      if (mp)
	best_mp = mp;
    }
  return best_mp;
}


msgdomain_ty*
msgdomain_alloc (const char *domain, bool use_hashtable)
{
  msgdomain_ty *mdp;

  mdp = (msgdomain_ty *) xmalloc (sizeof (msgdomain_ty));
  mdp->domain = domain;
  mdp->messages = message_list_alloc (use_hashtable);
  return mdp;
}


void
msgdomain_free (msgdomain_ty *mdp)
{
  message_list_free (mdp->messages);
  free (mdp);
}


msgdomain_list_ty *
msgdomain_list_alloc (bool use_hashtable)
{
  msgdomain_list_ty *mdlp;

  mdlp = (msgdomain_list_ty *) xmalloc (sizeof (msgdomain_list_ty));
  /* Put the default domain first, so that when we output it,
     we can omit the 'domain' directive.  */
  mdlp->nitems = 1;
  mdlp->nitems_max = 1;
  mdlp->item =
    (msgdomain_ty **) xmalloc (mdlp->nitems_max * sizeof (msgdomain_ty *));
  mdlp->item[0] = msgdomain_alloc (MESSAGE_DOMAIN_DEFAULT, use_hashtable);
  mdlp->use_hashtable = use_hashtable;
  mdlp->encoding = NULL;
  return mdlp;
}


void
msgdomain_list_free (msgdomain_list_ty *mdlp)
{
  size_t j;

  for (j = 0; j < mdlp->nitems; ++j)
    msgdomain_free (mdlp->item[j]);
  if (mdlp->item)
    free (mdlp->item);
  free (mdlp);
}


void
msgdomain_list_append (msgdomain_list_ty *mdlp, msgdomain_ty *mdp)
{
  if (mdlp->nitems >= mdlp->nitems_max)
    {
      size_t nbytes;

      mdlp->nitems_max = mdlp->nitems_max * 2 + 4;
      nbytes = mdlp->nitems_max * sizeof (msgdomain_ty *);
      mdlp->item = xrealloc (mdlp->item, nbytes);
    }
  mdlp->item[mdlp->nitems++] = mdp;
}


#if 0 /* unused */
void
msgdomain_list_append_list (msgdomain_list_ty *mdlp, msgdomain_list_ty *mdlp2)
{
  size_t j;

  for (j = 0; j < mdlp2->nitems; ++j)
    msgdomain_list_append (mdlp, mdlp2->item[j]);
}
#endif


message_list_ty *
msgdomain_list_sublist (msgdomain_list_ty *mdlp, const char *domain,
			bool create)
{
  size_t j;

  for (j = 0; j < mdlp->nitems; j++)
    if (strcmp (mdlp->item[j]->domain, domain) == 0)
      return mdlp->item[j]->messages;

  if (create)
    {
      msgdomain_ty *mdp = msgdomain_alloc (domain, mdlp->use_hashtable);
      msgdomain_list_append (mdlp, mdp);
      return mdp->messages;
    }
  else
    return NULL;
}


#if 0 /* unused */
message_ty *
msgdomain_list_search (msgdomain_list_ty *mdlp, const char *msgid)
{
  size_t j;

  for (j = 0; j < mdlp->nitems; ++j)
    {
      msgdomain_ty *mdp;
      message_ty *mp;

      mdp = mdlp->item[j];
      mp = message_list_search (mdp->messages, msgid);
      if (mp)
	return mp;
    }
  return NULL;
}
#endif


#if 0 /* unused */
message_ty *
msgdomain_list_search_fuzzy (msgdomain_list_ty *mdlp, const char *msgid)
{
  size_t j;
  double best_weight;
  message_ty *best_mp;

  best_weight = 0.6;
  best_mp = NULL;
  for (j = 0; j < mdlp->nitems; ++j)
    {
      msgdomain_ty *mdp;
      message_ty *mp;

      mdp = mdlp->item[j];
      mp = message_list_search_fuzzy_inner (mdp->messages, msgid, &best_weight);
      if (mp)
	best_mp = mp;
    }
  return best_mp;
}
#endif
