/* GNU gettext - internationalization aids
   Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.

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
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <ctype.h>
#include <stdio.h>

#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif

#ifdef HAVE_LOCALE_H
# include <locale.h>
#endif

#ifdef STDC_HEADERS
# include <stdlib.h>
#endif

#include "fstrcmp.h"
#include "message.h"
#include "system.h"
#include "error.h"
#include "libgettext.h"


/* Our regular abbreviation.  */
#define _(str) gettext (str)


/* These two variables control the output style of the message_print
   function.  Interface functions for them are to be used.  */
static int indent;
static int uniforum;
static int escape;

/* This variable controls the page width when printing messages.
   Defaults to PAGE_WIDTH if not set.  Zero (0) given to message_page_-
   width_set will result in no wrapping being performed.  */
static size_t page_width = PAGE_WIDTH;


/* Prototypes for local functions.  */
static void wrap PARAMS ((FILE *__fp, const char *__line_prefix,
			  const char *__name, const char *__value,
			  int do_wrap));
static void print_blank_line PARAMS ((FILE *__fp));
static void message_print PARAMS ((const message_ty *__mp, FILE *__fp,
				   const char *__domain, int blank_line,
				   int __debug));
static void message_print_obsolete PARAMS ((const message_ty *__mp, FILE *__fp,
					    const char *__domain,
					    int blank_line));
static int msgid_cmp PARAMS ((const void *__va, const void *__vb));
static int filepos_cmp PARAMS ((const void *__va, const void *__vb));
static const char *make_c_format_description_string PARAMS ((enum is_c_format,
							     int debug));
static const char *make_c_width_description_string PARAMS ((enum is_c_format));
static int significant_c_format_p PARAMS ((enum is_c_format __is_c_format));



message_ty *
message_alloc (msgid)
     char *msgid;
{
  message_ty *mp;

  mp = xmalloc (sizeof (message_ty));
  mp->msgid = msgid;
  mp->comment = NULL;
  mp->comment_dot = NULL;
  mp->filepos_count = 0;
  mp->filepos = NULL;
  mp->variant_count = 0;
  mp->variant = NULL;
  mp->used = 0;
  mp->obsolete = 0;
  mp->is_fuzzy = 0;
  mp->is_c_format = undecided;
  mp->do_wrap = undecided;
  return mp;
}


void
message_free (mp)
     message_ty *mp;
{
  size_t j;

  if (mp->comment != NULL)
    string_list_free (mp->comment);
  if (mp->comment_dot != NULL)
    string_list_free (mp->comment_dot);
  free ((char *) mp->msgid);
  for (j = 0; j < mp->variant_count; ++j)
    free ((char *) mp->variant[j].msgstr);
  if (mp->variant != NULL)
    free (mp->variant);
  for (j = 0; j < mp->filepos_count; ++j)
    free ((char *) mp->filepos[j].file_name);
  if (mp->filepos != NULL)
    free (mp->filepos);
  free (mp);
}


message_variant_ty *
message_variant_search (mp, domain)
     message_ty *mp;
     const char *domain;
{
  size_t j;
  message_variant_ty *mvp;

  for (j = 0; j < mp->variant_count; ++j)
    {
      mvp = &mp->variant[j];
      if (0 == strcmp (domain, mvp->domain))
	return mvp;
    }
  return 0;
}


void
message_variant_append (mp, domain, msgstr, pp)
     message_ty *mp;
     const char *domain;
     const char *msgstr;
     const lex_pos_ty *pp;
{
  size_t nbytes;
  message_variant_ty *mvp;

  nbytes = (mp->variant_count + 1) * sizeof (mp->variant[0]);
  mp->variant = xrealloc (mp->variant, nbytes);
  mvp = &mp->variant[mp->variant_count++];
  mvp->domain = domain;
  mvp->msgstr = msgstr;
  mvp->pos = *pp;
}


void
message_comment_append (mp, s)
     message_ty *mp;
     const char *s;
{
  if (mp->comment == NULL)
    mp->comment = string_list_alloc ();
  string_list_append (mp->comment, s);
}


void
message_comment_dot_append (mp, s)
     message_ty *mp;
     const char *s;
{
  if (mp->comment_dot == NULL)
    mp->comment_dot = string_list_alloc ();
  string_list_append (mp->comment_dot, s);
}


message_ty *
message_copy (mp)
     message_ty *mp;
{
  message_ty *result;
  size_t j;

  result = message_alloc (xstrdup (mp->msgid));

  for (j = 0; j < mp->variant_count; ++j)
    {
      message_variant_ty *mvp = &mp->variant[j];
      message_variant_append (result, mvp->domain, mvp->msgstr, &mvp->pos);
    }
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
  result->is_c_format = mp->is_c_format;
  result->do_wrap = mp->do_wrap;
  for (j = 0; j < mp->filepos_count; ++j)
    {
      lex_pos_ty *pp = &mp->filepos[j];
      message_comment_filepos (result, pp->file_name, pp->line_number);
    }
  return result;
}


message_ty *
message_merge (def, ref)
     message_ty *def;
     message_ty *ref;
{
  message_ty *result;
  const char *pot_date_ptr = NULL;
  size_t pot_date_len = 0;
  size_t j;

  /* Take the msgid from the reference.  When fuzzy matches are made,
     the definition will not be unique, but the reference will be -
     usually because it has a typo.  */
  result = message_alloc (xstrdup (ref->msgid));

  /* If msgid is the header entry (i.e., "") we find the
     POT-Creation-Date line in the reference.  */
  if (ref->msgid[0] == '\0')
    {
      pot_date_ptr = strstr (ref->variant[0].msgstr, "POT-Creation-Date:");
      if (pot_date_ptr != NULL)
	{
	  const char *endp;

	  pot_date_ptr += sizeof ("POT-Creation-Date:") - 1;

	  endp = strchr (pot_date_ptr, '\n');
	  if (endp == NULL)
	    {
	      char *extended;
	      endp = strchr (pot_date_ptr, '\0');
	      pot_date_len = (endp - pot_date_ptr) + 1;
	      extended = (char *) alloca (pot_date_len + 1);
	      stpcpy (stpcpy (extended, pot_date_ptr), "\n");
	      pot_date_ptr = extended;
	    }
	  else
	    pot_date_len = (endp - pot_date_ptr) + 1;

	  if (pot_date_len == 0)
	    pot_date_ptr = NULL;
	}
    }

  /* Take the variant list from the definition.  The msgstr of the
     refences will be empty, as they were generated by xgettext.  If
     we currently process the header entry we have to merge the msgstr
     by using the POT-Creation-Date field from the .pot file.  */
  for (j = 0; j < def->variant_count; ++j)
    {
      message_variant_ty *mvp = &def->variant[j];

      if (ref->msgid[0] == '\0')
	{
	  /* Oh, oh.  The header entry and we have something to fill in.  */
	  static const struct
	  {
	    const char *name;
	    size_t len;
	  } known_fields[] =
	  {
	    { "Project-Id-Version:", sizeof ("Project-Id-Version:") - 1 },
#define PROJECT_ID	0
	    { "POT-Creation-Date:", sizeof ("POT-Creation-Date:") - 1 },
#define POT_CREATION	1
	    { "PO-Revision-Date:", sizeof ("PO-Revision-Date:") - 1 },
#define PO_REVISION	2
	    { "Last-Translator:", sizeof ("Last-Translator:") - 1 },
#define LAST_TRANSLATOR	3
	    { "Language-Team:", sizeof ("Language-Team:") - 1 },
#define LANGUAGE_TEAM	4
	    { "MIME-Version:", sizeof ("MIME-Version:") - 1 },
#define MIME_VERSION	5
	    { "Content-Type:", sizeof ("Content-Type:") - 1 },
#define CONTENT_TYPE	6
	    { "Content-Transfer-Encoding:",
	      sizeof ("Content-Transfer-Encoding:") - 1 }
#define CONTENT_TRANSFER 7
	  };
#define UNKNOWN	8
	  struct
	  {
	    const char *string;
	    size_t len;
	  } header_fields[UNKNOWN + 1];
	  const char *cp;
	  char *newp;
	  size_t len, cnt;

	  /* Clear all fields.  */
	  memset (header_fields, '\0', sizeof (header_fields));

	  cp = mvp->msgstr;
	  while (*cp != '\0')
	    {
	      const char *endp = strchr (cp, '\n');
	      int terminated = endp != NULL;

	      if (!terminated)
		{
		  char *copy;
		  endp = strchr (cp, '\0');

		  len = endp - cp + 1;

		  copy = (char *) alloca (len + 1);
		  stpcpy (stpcpy (copy, cp), "\n");
		  cp = copy;
		}
	      else
		{
		  len = (endp - cp) + 1;
		  ++endp;
		}

	      /* Compare with any of the known fields.  */
	      for (cnt = 0;
		   cnt < sizeof (known_fields) / sizeof (known_fields[0]);
		   ++cnt)
		if (strncasecmp (cp, known_fields[cnt].name,
				 known_fields[cnt].len) == 0)
		  break;

	      if (cnt < sizeof (known_fields) / sizeof (known_fields[0]))
		{
		  header_fields[cnt].string = &cp[known_fields[cnt].len];
		  header_fields[cnt].len = len - known_fields[cnt].len;
		}
	      else
		{
		  /* It's an unknown field.  Append content to what is
		     already known.  */
		  char *extended = (char *) alloca (header_fields[UNKNOWN].len
						    + len + 1);
		  memcpy (extended, header_fields[UNKNOWN].string,
			  header_fields[UNKNOWN].len);
		  memcpy (&extended[header_fields[UNKNOWN].len], cp, len);
		  extended[header_fields[UNKNOWN].len + len] = '\0';
		  header_fields[UNKNOWN].string = extended;
		  header_fields[UNKNOWN].len += len;
		}

	      cp = endp;
	    }

	  if (pot_date_ptr != NULL)
	    {
	      header_fields[POT_CREATION].string = pot_date_ptr;
	      header_fields[POT_CREATION].len = pot_date_len;
	    }

	  /* Concatenate all the various fields.  */
	  len = 0;
	  for (cnt = 0; cnt < UNKNOWN; ++cnt)
	    if (header_fields[cnt].string != NULL)
	      len += known_fields[cnt].len + header_fields[cnt].len;
	  len += header_fields[UNKNOWN].len;

	  cp = newp = (char *) xmalloc (len + 1);
	  newp[len] = '\0';

#define IF_FILLED(idx)							      \
	  if (header_fields[idx].string)				      \
	    newp = stpncpy (stpcpy (newp, known_fields[idx].name),	      \
			    header_fields[idx].string, header_fields[idx].len)

	  IF_FILLED (PROJECT_ID);
	  IF_FILLED (POT_CREATION);
	  IF_FILLED (PO_REVISION);
	  IF_FILLED (LAST_TRANSLATOR);
	  IF_FILLED (LANGUAGE_TEAM);
	  IF_FILLED (MIME_VERSION);
	  IF_FILLED (CONTENT_TYPE);
	  IF_FILLED (CONTENT_TRANSFER);
	  if (header_fields[UNKNOWN].string != NULL)
	    stpcpy (newp, header_fields[UNKNOWN].string);

	  message_variant_append (result, mvp->domain, cp, &mvp->pos);
	}
      else
	message_variant_append (result, mvp->domain, mvp->msgstr, &mvp->pos);
    }

  /* Take the comments from the definition file.  There will be none at
     all in the reference file, as it was generated by xgettext.  */
  if (def->comment)
    for (j = 0; j < def->comment->nitems; ++j)
      message_comment_append (result, def->comment->item[j]);

  /* Take the dot comments from the reference file, as they are
     generated by xgettext.  Any in the definition file are old ones
     collected by previous runs of xgettext and msgmerge.  */
  if (ref->comment_dot)
    for (j = 0; j < ref->comment_dot->nitems; ++j)
      message_comment_dot_append (result, ref->comment_dot->item[j]);

  /* The flags are mixed in a special way.  Some informations come
     from the reference message (such as format/no-format), others
     come from the definition file (fuzzy or not).  */
  result->is_fuzzy = def->is_fuzzy;
  result->is_c_format = ref->is_c_format;
  result->do_wrap = ref->do_wrap;

  /* Take the file position comments from the reference file, as they
     are generated by xgettext.  Any in the definition file are old ones
     collected by previous runs of xgettext and msgmerge.  */
  for (j = 0; j < ref->filepos_count; ++j)
    {
      lex_pos_ty *pp = &ref->filepos[j];
      message_comment_filepos (result, pp->file_name, pp->line_number);
    }

  /* All done, return the merged message to the caller.  */
  return result;
}


void
message_comment_filepos (mp, name, line)
     message_ty *mp;
     const char *name;
     size_t line;
{
  size_t nbytes;
  lex_pos_ty *pp;
  int min, max;
  int j;

  /* See if we have this position already.  They are kept in sorted
     order, so use a binary chop.  */
  /* FIXME: use bsearch */
  min = 0;
  max = (int) mp->filepos_count - 1;
  while (min <= max)
    {
      int mid;
      int cmp;

      mid = (min + max) / 2;
      pp = &mp->filepos[mid];
      cmp = strcmp (pp->file_name, name);
      if (cmp == 0)
	cmp = (int) pp->line_number - line;
      if (cmp == 0)
	return;
      if (cmp < 0)
	min = mid + 1;
      else
	max = mid - 1;
    }

  /* Extend the list so that we can add an position to it.  */
  nbytes = (mp->filepos_count + 1) * sizeof (mp->filepos[0]);
  mp->filepos = xrealloc (mp->filepos, nbytes);

  /* Shuffle the rest of the list up one, so that we can insert the
     position at ``min''.  */
  /* FIXME: use memmove */
  for (j = mp->filepos_count; j > min; --j)
    mp->filepos[j] = mp->filepos[j - 1];
  mp->filepos_count++;

  /* Insert the postion into the empty slot.  */
  pp = &mp->filepos[min];
  pp->file_name = xstrdup (name);
  pp->line_number = line;
}


void
message_print_style_indent ()
{
  indent = 1;
}


void
message_print_style_uniforum ()
{
  uniforum = 1;
}


void
message_print_style_escape (flag)
     int flag;
{
  escape = (flag != 0);
}


message_list_ty *
message_list_alloc ()
{
  message_list_ty *mlp;

  mlp = xmalloc (sizeof (message_list_ty));
  mlp->nitems = 0;
  mlp->nitems_max = 0;
  mlp->item = 0;
  return mlp;
}


void
message_list_append (mlp, mp)
     message_list_ty *mlp;
     message_ty *mp;
{
  if (mlp->nitems >= mlp->nitems_max)
    {
      size_t nbytes;

      mlp->nitems_max = mlp->nitems_max * 2 + 4;
      nbytes = mlp->nitems_max * sizeof (message_ty *);
      mlp->item = xrealloc (mlp->item, nbytes);
    }
  mlp->item[mlp->nitems++] = mp;
}


void
message_list_delete_nth (mlp, n)
     message_list_ty *mlp;
     size_t n;
{
  size_t j;

  if (n >= mlp->nitems)
    return;
  message_free (mlp->item[n]);
  for (j = n + 1; j < mlp->nitems; ++j)
    mlp->item[j - 1] = mlp->item[j];
  mlp->nitems--;
}


message_ty *
message_list_search (mlp, msgid)
     message_list_ty *mlp;
     const char *msgid;
{
  size_t j;

  for (j = 0; j < mlp->nitems; ++j)
    {
      message_ty *mp;

      mp = mlp->item[j];
      if (0 == strcmp (msgid, mp->msgid))
	return mp;
    }
  return 0;
}


message_ty *
message_list_search_fuzzy (mlp, msgid)
     message_list_ty *mlp;
     const char *msgid;
{
  size_t j;
  double best_weight;
  message_ty *best_mp;

  best_weight = 0.6;
  best_mp = NULL;
  for (j = 0; j < mlp->nitems; ++j)
    {
      size_t k;
      double weight;
      message_ty *mp;

      mp = mlp->item[j];

      for (k = 0; k < mp->variant_count; ++k)
	if (mp->variant[k].msgstr != NULL && mp->variant[k].msgstr[0] != '\0')
	  break;
      if (k >= mp->variant_count)
	continue;

      weight = fstrcmp (msgid, mp->msgid);
      if (weight > best_weight)
	{
	  best_weight = weight;
	  best_mp = mp;
	}
    }
  return best_mp;
}


void
message_list_free (mlp)
     message_list_ty *mlp;
{
  size_t j;

  for (j = 0; j < mlp->nitems; ++j)
    message_free (mlp->item[j]);
  if (mlp->item)
    free (mlp->item);
  free (mlp);
}


/* Local functions.  */

static void
wrap (fp, line_prefix, name, value, do_wrap)
     FILE *fp;
     const char *line_prefix;
     const char *name;
     const char *value;
     int do_wrap;
{
  const char *s;
  int first_line;
  /* The \a and \v escapes were added by the ANSI C Standard.  Prior
     to the Standard, most compilers did not have them.  Because we
     need the same program on all platforms we don't provide support
     for them here.  */
  static const char escapes[] = "\b\f\n\r\t";
  static const char escape_names[] = "bfnrt";

  /* The empty string is a special case.  */
  if (*value == '\0')
    {
      if (line_prefix != NULL)
	fputs (line_prefix, fp);
      fputs (name, fp);
      putc (indent ? '\t' : ' ', fp);
      fputs ("\"\"\n", fp);
      return;
    }

  s = value;
  first_line = 1;
  while (*s)
    {
      const char *ep;
      int ocol;

      /* The line starts with different things depending on whether it
         is the first line, and if we are using the indented style.  */
      if (first_line)
	{
	  ocol = strlen (name) + (line_prefix ? strlen (line_prefix) : 0);
	  if (indent && ocol < 8)
	    ocol = 8;
	  else
	    ++ocol;
	}
      else
	ocol = (indent ? 8 : 0);

      /* Allow room for the opening quote character. */
      ++ocol;

      /* Work out how many characters from the string will fit on a
         line.  Natural breaks occur at embedded newline characters.  */
      for (ep = s; *ep; ++ep)
	{
	  const char *esc;
	  int cw;
	  int c;

	  c = (unsigned char) *ep;
	  /* FIXME This is the wrong locale.  While message_list_print
	     set the "C" locale for LC_CTYPE, the need is to use the
	     correct locale for the file's contents.  */
	  esc = strchr (escapes, c);
	  if (esc == NULL && (!escape || isprint (c)))
	    cw = 1 + (c == '\\' || c == '"');
	  else
	    cw = esc != NULL ? 2 : 4;
	  /* Allow 1 character for the closing quote.  */
	  if (ocol + cw >= (do_wrap == no ? INT_MAX : page_width))
	    break;
	  ocol += cw;
	  if (c == '\n')
	    {
	      ++ep;
	      break;
	    }
	}

      /* The above loop detects if a line is too long.  If it is too
	 long, see if there is a better place to break the line.  */
      if (*ep)
	{
	  const char *bp;

	  for (bp = ep; bp > s; --bp)
	    if (bp[-1] == ' ' || bp[-1] == '\n')
	      {
		ep = bp;
		break;
	      }
	}

      /* If this is the first line, and we are not using the indented
         style, and the line would wrap, then use an empty first line
         and restart.  */
      if (first_line && !indent && *ep != '\0')
	{
	  fprintf (fp, "%s%s \"\"\n", line_prefix ? line_prefix : "", name);
	  s = value;
	  first_line = 0;
	  continue;
	}

      /* Print the beginning of the line.  This will depend on whether
	 this is the first line, and if the indented style is being
	 used.  */
      if (first_line)
	{
	  first_line = 0;
	  if (line_prefix != NULL)
	    fputs (line_prefix, fp);
	  fputs (name, fp);
	  putc (indent ? '\t' : ' ', fp);
	}
      else
	{
	  if (line_prefix != NULL)
	    fputs (line_prefix, fp);
	  if (indent)
	    putc ('\t', fp);
	}

      /* Print the body of the line.  C escapes are used for
	 unprintable characters.  */
      putc ('"', fp);
      while (s < ep)
	{
	  const char *esc;
	  int c;

	  c = (unsigned char) *s++;
	  /* FIXME This is the wrong locale.  While message_list_print
	     set the "C" locale for LC_CTYPE, the need is to use the
	     correct locale for the file's contents.  */
	  esc = strchr (escapes, c);
	  if (esc == NULL && (!escape || isprint (c)))
	    {
	      if (c == '\\' || c == '"')
		putc ('\\', fp);
	      putc (c, fp);
	    }
	  else if (esc != NULL)
	    {
	      c = escape_names[esc - escapes];

	      putc ('\\', fp);
	      putc (c, fp);

	      /* We warn about any use of escape sequences beside
		 '\n' and '\t'.  */
	      if (c != 'n' && c != 't')
		error (0, 0, _("\
internationalized messages should not contain the `\\%c' escape sequence"),
		       c);
	    }
	  else
	    fprintf (fp, "\\%3.3o", c);
	}
      fputs ("\"\n", fp);
    }
}


static void
print_blank_line (fp)
     FILE *fp;
{
  if (uniforum)
    fputs ("#\n", fp);
  else
    putc ('\n', fp);
}


static void
message_print (mp, fp, domain, blank_line, debug)
     const message_ty *mp;
     FILE *fp;
     const char *domain;
     int blank_line;
     int debug;
{
  message_variant_ty *mvp;
  int first;
  size_t j;

  /* Find the relevant message variant.  If there isn't one, remember
     this using a NULL pointer.  */
  mvp = NULL;
  first = 0;

  for (j = 0; j < mp->variant_count; ++j)
    {
      if (strcmp (domain, mp->variant[j].domain) == 0)
	{
	  mvp = &mp->variant[j];
	  first = (j == 0);
	  break;
	}
    }

  /* Separate messages with a blank line.  Uniforum doesn't like blank
     lines, so use an empty comment (unless there already is one).  */
  if (blank_line && (!uniforum
		     || mp->comment == NULL
		     || mp->comment->nitems == 0
		     || mp->comment->item[0][0] != '\0'))
    print_blank_line (fp);

  /* The first variant of a message will have the comments attached to
     it.  We can't attach them to all variants in case we are read in
     again, multiplying the number of comment lines.  Usually there is
     only one variant.  */
  if (first)
    {
      if (mp->comment != NULL)
	for (j = 0; j < mp->comment->nitems; ++j)
	  {
	    const unsigned char *s = mp->comment->item[j];
	    do
	      {
		const unsigned char *e;
		putc ('#', fp);
	       /* FIXME This is the wrong locale.  While
		  message_list_print set the "C" locale for LC_CTYPE,
		  the need to use the correct locale for the file's
		  contents.  */
		if (*s != '\0' && !isspace (*s))
		  putc (' ', fp);
		e = strchr (s, '\n');
		if (e == NULL)
		  {
		    fputs (s, fp);
		    s = NULL;
		  }
		else
		  {
		    fwrite (s, 1, e - s, fp);
		    s = e + 1;
		  }
		putc ('\n', fp);
	      }
	    while (s != NULL);
	  }

      if (mp->comment_dot != NULL)
	for (j = 0; j < mp->comment_dot->nitems; ++j)
	  {
	    const unsigned char *s = mp->comment_dot->item[j];
	    putc ('#', fp);
	    putc ('.', fp);
	    /* FIXME This is the wrong locale.  While
	       message_list_print set the "C" locale for LC_CTYPE, the
	       need to use the correct locale for the file's contents.  */
	    if (*s && !isspace (*s))
	      putc (' ', fp);
	    fputs (s, fp);
	    putc ('\n', fp);
	  }
    }

  /* Print the file position comments for every domain.  This will
     help a human who is trying to navigate the sources.  There is no
     problem of getting repeat positions, because duplicates are
     checked for.  */
  if (mp->filepos_count != 0)
    {
      if (uniforum)
	for (j = 0; j < mp->filepos_count; ++j)
	  {
	    lex_pos_ty *pp = &mp->filepos[j];
	    char *cp = pp->file_name;
	    while (cp[0] == '.' && cp[1] == '/')
	      cp += 2;
	    /* There are two Sun formats to choose from: SunOS and
	       Solaris.  Use the Solaris form here.  */
	    fprintf (fp, "# File: %s, line: %ld\n",
		     cp, (long) pp->line_number);
	  }
      else
	{
	  size_t column;

	  fputs ("#:", fp);
	  column = 2;
	  for (j = 0; j < mp->filepos_count; ++j)
	    {
	      lex_pos_ty *pp;
	      char buffer[20];
	      char *cp;
	      size_t len;

	      pp = &mp->filepos[j];
	      cp = pp->file_name;
	      while (cp[0] == '.' && cp[1] == '/')
		cp += 2;
	      sprintf (buffer, "%ld", (long) pp->line_number);
	      len = strlen (cp) + strlen (buffer) + 2;
	      if (column > 2 && column + len >= page_width)
		{
		  fputs ("\n#:", fp);
		  column = 2;
		}
	      fprintf (fp, " %s:%s", cp, buffer);
	      column += len;
	    }
	  putc ('\n', fp);
	}
    }

  /* Print flag information in special comment.  */
  if (first && ((mp->is_fuzzy && mvp != NULL && mvp->msgstr[0] != '\0')
		|| significant_c_format_p (mp->is_c_format)
		|| mp->do_wrap == no))
    {
      int first_flag = 1;

      putc ('#', fp);
      putc (',', fp);

      /* We don't print the fuzzy flag if the msgstr is empty.  This
	 might be introduced by the user but we want to normalize the
	 output.  */
      if (mp->is_fuzzy && mvp != NULL && mvp->msgstr[0] != '\0')
	{
	  fputs (" fuzzy", fp);
	  first_flag = 0;
	}

      if (significant_c_format_p (mp->is_c_format))
	{
	  if (!first_flag)
	    putc (',', fp);

	  fputs (make_c_format_description_string (mp->is_c_format, debug),
		 fp);
	  first_flag = 0;
	}

      if (mp->do_wrap == no)
	{
	  if (!first_flag)
	    putc (',', fp);

	  fputs (make_c_width_description_string (mp->do_wrap), fp);
	  first_flag = 0;
	}

      putc ('\n', fp);
    }

  /* Print each of the message components.  Wrap them nicely so they
     are as readable as possible.  If there is no recorded msgstr for
     this domain, emit an empty string.  */
  wrap (fp, NULL, "msgid", mp->msgid, mp->do_wrap);
  wrap (fp, NULL, "msgstr", mvp ? mvp->msgstr : "", mp->do_wrap);
}


static void
message_print_obsolete (mp, fp, domain, blank_line)
     const message_ty *mp;
     FILE *fp;
     const char *domain;
     int blank_line;
{
  message_variant_ty *mvp;
  size_t j;

  /* Find the relevant message variant.  If there isn't one, remember
     this using a NULL pointer.  */
  mvp = NULL;

  for (j = 0; j < mp->variant_count; ++j)
    {
      if (strcmp (domain, mp->variant[j].domain) == 0)
	{
	  mvp = &mp->variant[j];
	  break;
	}
    }

  /* If no msgstr is found or it is the empty string we print nothing.  */
  if (mvp == NULL || mvp->msgstr[0] == '\0')
    return;

  /* Separate messages with a blank line.  Uniforum doesn't like blank
     lines, so use an empty comment (unless there already is one).  */
  if (blank_line)
    print_blank_line (fp);

  /* Print translator comment if available.  */
  if (mp->comment)
    for (j = 0; j < mp->comment->nitems; ++j)
      {
	const unsigned char *s = mp->comment->item[j];
	do
	  {
	    const unsigned char *e;
	    putc ('#', fp);
	    /* FIXME This is the wrong locale.  While
	       message_list_print set the "C" locale for LC_CTYPE, the
	       need to use the correct locale for the file's contents.  */
	    if (*s != '\0' && !isspace (*s))
	      putc (' ', fp);
	    e = strchr (s, '\n');
	    if (e == NULL)
	      {
		fputs (s, fp);
		s = NULL;
	      }
	    else
	      {
		fwrite (s, 1, e - s, fp);
		s = e + 1;
	      }
	    putc ('\n', fp);
	  }
	while (s != NULL);
      }

  /* Print flag information in special comment.  */
  if (mp->is_fuzzy)
    {
      int first = 1;

      putc ('#', fp);
      putc (',', fp);

      if (mp->is_fuzzy)
	{
	  fputs (" fuzzy", fp);
	  first = 0;
	}

      putc ('\n', fp);
    }

  /* Print each of the message components.  Wrap them nicely so they
     are as readable as possible.  */
  wrap (fp, "#~ ", "msgid", mp->msgid, mp->do_wrap);
  wrap (fp, "#~ ", "msgstr", mvp->msgstr, mp->do_wrap);
}


void
message_list_print (mlp, filename, force, debug)
     message_list_ty *mlp;
     const char *filename;
     int force;
     int debug;
{
  FILE *fp;
  size_t j, k;
  string_list_ty *dl;
  int blank_line;
#ifdef HAVE_SETLOCALE
  char *old_locale;
#endif

  /* We will not write anything if we have no message or only the
     header entry.  */
  if (force == 0
      && (mlp->nitems == 0
	  || (mlp->nitems == 1 && *mlp->item[0]->msgid == '\0')))
    return;

  /* Build the list of domains.  */
  dl = string_list_alloc ();
  for (j = 0; j < mlp->nitems; ++j)
    {
      message_ty *mp = mlp->item[j];
      for (k = 0; k < mp->variant_count; ++k)
	string_list_append_unique (dl, mp->variant[k].domain);
    }

  /* Open the output file.  */
  if (filename != NULL && strcmp (filename, "-") != 0
      && strcmp (filename, "/dev/stdout") != 0)
    {
      fp = fopen (filename, "w");
      if (fp == NULL)
	error (EXIT_FAILURE, errno, _("cannot create output file \"%s\""),
	       filename);
    }
  else
    {
      fp = stdout;
      /* xgettext:no-c-format */
      filename = _("standard output");
    }

#ifdef HAVE_SETLOCALE
  /* FIXME This is the wrong locale.  The program is currently set for
     the user's native language locale, for the error messages.  This
     code sets it to the "C" locale, but that isn't right either.  The
     need is to use the correct locale for the file's contents.  */
  old_locale = setlocale (LC_CTYPE, "C");
  if (old_locale)
    old_locale = xstrdup (old_locale);
#endif

  /* Write out the messages for each domain.  */
  blank_line = 0;
  for (k = 0; k < dl->nitems; ++k)
    {
      /* If there is only one domain, and that domain is the default,
	 don't bother emitting the domain name, because it is the
	 default.  */
      if (dl->nitems != 1 || strcmp (dl->item[0], MESSAGE_DOMAIN_DEFAULT) != 0)
	{
	  if (blank_line)
	    print_blank_line (fp);
	  fprintf (fp, "domain \"%s\"\n", dl->item[k]);
	  blank_line = 1;
	}

      /* Write out each of the messages for this domain.  */
      for (j = 0; j < mlp->nitems; ++j)
	if (mlp->item[j]->obsolete == 0)
	  {
	    message_print (mlp->item[j], fp, dl->item[k], blank_line, debug);
	    blank_line = 1;
	  }

      /* Write out each of the obsolete messages for this domain.  */
      for (j = 0; j < mlp->nitems; ++j)
	if (mlp->item[j]->obsolete != 0)
	  {
	    message_print_obsolete (mlp->item[j], fp, dl->item[k], blank_line);
	    blank_line = 1;
	  }
    }
  string_list_free (dl);

  /* Restore the old locale.  Do this before emitting error messages,
     so that the correct locale is used for the error.  (Ideally,
     error should ensure this before calling gettext for the format
     string.)  */
#ifdef HAVE_SETLOCALE
  if (old_locale)
    {
      setlocale (LC_CTYPE, old_locale);
      free (old_locale);
    }
#endif

  /* Make sure nothing went wrong.  */
  if (fflush (fp))
    error (EXIT_FAILURE, errno, _("error while writing \"%s\" file"),
	   filename);
  fclose (fp);
}


static int
msgid_cmp (va, vb)
     const void *va;
     const void *vb;
{
  const message_ty *a = *(const message_ty **) va;
  const message_ty *b = *(const message_ty **) vb;
#ifdef HAVE_STRCOLL
  return strcoll (a->msgid, b->msgid);
#else
  return strcmp (a->msgid, b->msgid);
#endif
}


void
message_list_sort_by_msgid (mlp)
     message_list_ty *mlp;
{
  /* FIXME This is the wrong locale.  The program is currently set for
     the user's native language locale, for the error messages.  This
     code sets it to the "C" locale, but that isn't right either.  The
     need is to use the correct locale for the file's contents.  */
#ifdef HAVE_SETLOCALE
  char *tmp = setlocale (LC_COLLATE, "C");
  if (tmp)
    tmp = xstrdup (tmp);
#endif
  qsort (mlp->item, mlp->nitems, sizeof (mlp->item[0]), msgid_cmp);
#ifdef HAVE_SETLOCALE
  if (tmp)
    {
      setlocale (LC_COLLATE, tmp);
      free (tmp);
    }
#endif
}


static int
filepos_cmp (va, vb)
     const void *va;
     const void *vb;
{
  const message_ty *a = *(const message_ty **) va;
  const message_ty *b = *(const message_ty **) vb;
  int cmp;

  /* No filepos is smaller than any other filepos.  */
  if (a->filepos_count == 0)
  {
    if (b->filepos_count != 0)
      return -1;
  }
  if (b->filepos_count == 0)
    return 1;

  /* Compare on the file names...  */
  cmp = strcmp (a->filepos[0].file_name, b->filepos[0].file_name);
  if (cmp != 0)
  	return cmp;

  /* If they are equal, compare on the line numbers...  */
  cmp = a->filepos[0].line_number - b->filepos[0].line_number;
  if (cmp != 0)
	return cmp;

  /* If they are equal, compare on the msgid strings.  */
#ifdef HAVE_STRCOLL
  return strcoll (a->msgid, b->msgid);
#else
  return strcmp (a->msgid, b->msgid);
#endif
}


void
message_list_sort_by_filepos (mlp)
    message_list_ty *mlp;
{
  /* FIXME This is the wrong locale.  The program is currently set for
     the user's native language locale, for the error messages.  This
     code sets it to the "C" locale, but that isn't right either.  The
     need is to use the correct locale for the file's contents.  */
#ifdef HAVE_SETLOCALE
  char *tmp = setlocale (LC_COLLATE, "C");
  if (tmp)
    tmp = xstrdup (tmp);
#endif
  qsort (mlp->item, mlp->nitems, sizeof (mlp->item[0]), filepos_cmp);
#ifdef HAVE_SETLOCALE
  if (tmp)
    {
      setlocale (LC_COLLATE, tmp);
      free (tmp);
    }
#endif
}


enum is_c_format
parse_c_format_description_string (s)
     const char *s;
{
  if (strstr (s, "no-c-format") != NULL)
    return no;
  else if (strstr (s, "impossible-c-format") != NULL)
    return impossible;
  else if (strstr (s, "possible-c-format") != NULL)
    return possible;
  else if (strstr (s, "c-format") != NULL)
    return yes;
  return undecided;
}


enum is_c_format
parse_c_width_description_string (s)
     const char *s;
{
  if (strstr (s, "no-wrap") != NULL)
    return no;
  else if (strstr (s, "wrap") != NULL)
    return yes;
  return undecided;
}


static const char *
make_c_format_description_string (is_c_format, debug)
     enum is_c_format is_c_format;
     int debug;
{
  const char *result = NULL;

  switch (is_c_format)
    {
    case possible:
      if (debug)
	{
	  result = " possible-c-format";
	  break;
	}
      /* FALLTHROUGH */
    case yes:
      result = " c-format";
      break;
    case impossible:
      result = " impossible-c-format";
      break;
    case no:
      result = " no-c-format";
      break;
    case undecided:
      result = " undecided";
      break;
    default:
      abort ();
    }

  return result;
}


static const char *
make_c_width_description_string (do_wrap)
     enum is_c_format do_wrap;
{
  const char *result = NULL;

  switch (do_wrap)
    {
    case yes:
      result = " wrap";
      break;
    case no:
      result = " no-wrap";
      break;
    default:
      abort ();
    }

  return result;
}


int
possible_c_format_p (is_c_format)
     enum is_c_format is_c_format;
{
  return is_c_format == possible || is_c_format == yes;
}


static int
significant_c_format_p (is_c_format)
     enum is_c_format is_c_format;
{
  return is_c_format != undecided && is_c_format != impossible;
}


void
message_page_width_set (n)
     size_t n;
{
  if (n == 0)
    {
      page_width = INT_MAX;
      return;
    }

  if (n < 20)
    n = 20;

  page_width = n;
}
