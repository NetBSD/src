/* Symbol, variable and name lookup.
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
#include <elf.h>
#include <string.h>

/* Compare the given input string and length against a table of known C storage
   qualifier keywords.  We just ignore these in ctf_lookup_by_name, below.  To
   do this quickly, we use a pre-computed Perfect Hash Function similar to the
   technique originally described in the classic paper:

   R.J. Cichelli, "Minimal Perfect Hash Functions Made Simple",
   Communications of the ACM, Volume 23, Issue 1, January 1980, pp. 17-19.

   For an input string S of length N, we use hash H = S[N - 1] + N - 105, which
   for the current set of qualifiers yields a unique H in the range [0 .. 20].
   The hash can be modified when the keyword set changes as necessary.  We also
   store the length of each keyword and check it prior to the final strcmp().

   TODO: just use gperf.  */

static int
isqualifier (const char *s, size_t len)
{
  static const struct qual
  {
    const char *q_name;
    size_t q_len;
  } qhash[] = {
    {"static", 6}, {"", 0}, {"", 0}, {"", 0},
    {"volatile", 8}, {"", 0}, {"", 0}, {"", 0}, {"", 0},
    {"", 0}, {"auto", 4}, {"extern", 6}, {"", 0}, {"", 0},
    {"", 0}, {"", 0}, {"const", 5}, {"register", 8},
    {"", 0}, {"restrict", 8}, {"_Restrict", 9}
  };

  int h = s[len - 1] + (int) len - 105;
  const struct qual *qp = &qhash[h];

  return (h >= 0 && (size_t) h < sizeof (qhash) / sizeof (qhash[0])
	  && (size_t) len == qp->q_len &&
	  strncmp (qp->q_name, s, qp->q_len) == 0);
}

/* Attempt to convert the given C type name into the corresponding CTF type ID.
   It is not possible to do complete and proper conversion of type names
   without implementing a more full-fledged parser, which is necessary to
   handle things like types that are function pointers to functions that
   have arguments that are function pointers, and fun stuff like that.
   Instead, this function implements a very simple conversion algorithm that
   finds the things that we actually care about: structs, unions, enums,
   integers, floats, typedefs, and pointers to any of these named types.  */

ctf_id_t
ctf_lookup_by_name (ctf_file_t *fp, const char *name)
{
  static const char delimiters[] = " \t\n\r\v\f*";

  const ctf_lookup_t *lp;
  const char *p, *q, *end;
  ctf_id_t type = 0;
  ctf_id_t ntype, ptype;

  if (name == NULL)
    return (ctf_set_errno (fp, EINVAL));

  for (p = name, end = name + strlen (name); *p != '\0'; p = q)
    {
      while (isspace (*p))
	p++;			/* Skip leading whitespace.  */

      if (p == end)
	break;

      if ((q = strpbrk (p + 1, delimiters)) == NULL)
	q = end;		/* Compare until end.  */

      if (*p == '*')
	{
	  /* Find a pointer to type by looking in fp->ctf_ptrtab.
	     If we can't find a pointer to the given type, see if
	     we can compute a pointer to the type resulting from
	     resolving the type down to its base type and use
	     that instead.  This helps with cases where the CTF
	     data includes "struct foo *" but not "foo_t *" and
	     the user tries to access "foo_t *" in the debugger.

	     TODO need to handle parent containers too.  */

	  ntype = fp->ctf_ptrtab[LCTF_TYPE_TO_INDEX (fp, type)];
	  if (ntype == 0)
	    {
	      ntype = ctf_type_resolve_unsliced (fp, type);
	      if (ntype == CTF_ERR
		  || (ntype =
		      fp->ctf_ptrtab[LCTF_TYPE_TO_INDEX (fp, ntype)]) == 0)
		{
		  (void) ctf_set_errno (fp, ECTF_NOTYPE);
		  goto err;
		}
	    }

	  type = LCTF_INDEX_TO_TYPE (fp, ntype, (fp->ctf_flags & LCTF_CHILD));

	  q = p + 1;
	  continue;
	}

      if (isqualifier (p, (size_t) (q - p)))
	continue;		/* Skip qualifier keyword.  */

      for (lp = fp->ctf_lookups; lp->ctl_prefix != NULL; lp++)
	{
	  /* TODO: This is not MT-safe.  */
	  if ((lp->ctl_prefix[0] == '\0' ||
	       strncmp (p, lp->ctl_prefix, (size_t) (q - p)) == 0) &&
	      (size_t) (q - p) >= lp->ctl_len)
	    {
	      for (p += lp->ctl_len; isspace (*p); p++)
		continue;	/* Skip prefix and next whitespace.  */

	      if ((q = strchr (p, '*')) == NULL)
		q = end;	/* Compare until end.  */

	      while (isspace (q[-1]))
		q--;		/* Exclude trailing whitespace.  */

	      /* Expand and/or allocate storage for a slice of the name, then
		 copy it in.  */

	      if (fp->ctf_tmp_typeslicelen >= (size_t) (q - p) + 1)
		{
		  memcpy (fp->ctf_tmp_typeslice, p, (size_t) (q - p));
		  fp->ctf_tmp_typeslice[(size_t) (q - p)] = '\0';
		}
	      else
		{
		  free (fp->ctf_tmp_typeslice);
		  fp->ctf_tmp_typeslice = xstrndup (p, (size_t) (q - p));
		  if (fp->ctf_tmp_typeslice == NULL)
		    {
		      (void) ctf_set_errno (fp, ENOMEM);
		      return CTF_ERR;
		    }
		}

	      if ((type = ctf_lookup_by_rawhash (fp, lp->ctl_hash,
						 fp->ctf_tmp_typeslice)) == 0)
		{
		  (void) ctf_set_errno (fp, ECTF_NOTYPE);
		  goto err;
		}

	      break;
	    }
	}

      if (lp->ctl_prefix == NULL)
	{
	  (void) ctf_set_errno (fp, ECTF_NOTYPE);
	  goto err;
	}
    }

  if (*p != '\0' || type == 0)
    return (ctf_set_errno (fp, ECTF_SYNTAX));

  return type;

err:
  if (fp->ctf_parent != NULL
      && (ptype = ctf_lookup_by_name (fp->ctf_parent, name)) != CTF_ERR)
    return ptype;

  return CTF_ERR;
}

typedef struct ctf_lookup_var_key
{
  ctf_file_t *clvk_fp;
  const char *clvk_name;
} ctf_lookup_var_key_t;

/* A bsearch function for variable names.  */

static int
ctf_lookup_var (const void *key_, const void *memb_)
{
  const ctf_lookup_var_key_t *key = key_;
  const ctf_varent_t *memb = memb_;

  return (strcmp (key->clvk_name, ctf_strptr (key->clvk_fp, memb->ctv_name)));
}

/* Given a variable name, return the type of the variable with that name.  */

ctf_id_t
ctf_lookup_variable (ctf_file_t *fp, const char *name)
{
  ctf_varent_t *ent;
  ctf_lookup_var_key_t key = { fp, name };

  /* This array is sorted, so we can bsearch for it.  */

  ent = bsearch (&key, fp->ctf_vars, fp->ctf_nvars, sizeof (ctf_varent_t),
		 ctf_lookup_var);

  if (ent == NULL)
    {
      if (fp->ctf_parent != NULL)
	return ctf_lookup_variable (fp->ctf_parent, name);

      return (ctf_set_errno (fp, ECTF_NOTYPEDAT));
    }

  return ent->ctv_type;
}

/* Given a symbol table index, return the name of that symbol from the secondary
   string table, or the null string (never NULL).  */
const char *
ctf_lookup_symbol_name (ctf_file_t *fp, unsigned long symidx)
{
  const ctf_sect_t *sp = &fp->ctf_symtab;
  Elf64_Sym sym, *gsp;

  if (sp->cts_data == NULL)
    {
      ctf_set_errno (fp, ECTF_NOSYMTAB);
      return _CTF_NULLSTR;
    }

  if (symidx >= fp->ctf_nsyms)
    {
      ctf_set_errno (fp, EINVAL);
      return _CTF_NULLSTR;
    }

  if (sp->cts_entsize == sizeof (Elf32_Sym))
    {
      const Elf32_Sym *symp = (Elf32_Sym *) sp->cts_data + symidx;
      gsp = ctf_sym_to_elf64 (symp, &sym);
    }
  else
      gsp = (Elf64_Sym *) sp->cts_data + symidx;

  if (gsp->st_name < fp->ctf_str[CTF_STRTAB_1].cts_len)
    return (const char *) fp->ctf_str[CTF_STRTAB_1].cts_strs + gsp->st_name;

  return _CTF_NULLSTR;
}

/* Given a symbol table index, return the type of the data object described
   by the corresponding entry in the symbol table.  */

ctf_id_t
ctf_lookup_by_symbol (ctf_file_t *fp, unsigned long symidx)
{
  const ctf_sect_t *sp = &fp->ctf_symtab;
  ctf_id_t type;

  if (sp->cts_data == NULL)
    return (ctf_set_errno (fp, ECTF_NOSYMTAB));

  if (symidx >= fp->ctf_nsyms)
    return (ctf_set_errno (fp, EINVAL));

  if (sp->cts_entsize == sizeof (Elf32_Sym))
    {
      const Elf32_Sym *symp = (Elf32_Sym *) sp->cts_data + symidx;
      if (ELF32_ST_TYPE (symp->st_info) != STT_OBJECT)
	return (ctf_set_errno (fp, ECTF_NOTDATA));
    }
  else
    {
      const Elf64_Sym *symp = (Elf64_Sym *) sp->cts_data + symidx;
      if (ELF64_ST_TYPE (symp->st_info) != STT_OBJECT)
	return (ctf_set_errno (fp, ECTF_NOTDATA));
    }

  if (fp->ctf_sxlate[symidx] == -1u)
    return (ctf_set_errno (fp, ECTF_NOTYPEDAT));

  type = *(uint32_t *) ((uintptr_t) fp->ctf_buf + fp->ctf_sxlate[symidx]);
  if (type == 0)
    return (ctf_set_errno (fp, ECTF_NOTYPEDAT));

  return type;
}

/* Return the pointer to the internal CTF type data corresponding to the
   given type ID.  If the ID is invalid, the function returns NULL.
   This function is not exported outside of the library.  */

const ctf_type_t *
ctf_lookup_by_id (ctf_file_t **fpp, ctf_id_t type)
{
  ctf_file_t *fp = *fpp;	/* Caller passes in starting CTF container.  */
  ctf_id_t idx;

  if ((fp->ctf_flags & LCTF_CHILD) && LCTF_TYPE_ISPARENT (fp, type)
      && (fp = fp->ctf_parent) == NULL)
    {
      (void) ctf_set_errno (*fpp, ECTF_NOPARENT);
      return NULL;
    }

  /* If this container is writable, check for a dynamic type.  */

  if (fp->ctf_flags & LCTF_RDWR)
    {
      ctf_dtdef_t *dtd;

      if ((dtd = ctf_dynamic_type (fp, type)) != NULL)
	{
	  *fpp = fp;
	  return &dtd->dtd_data;
	}
      (void) ctf_set_errno (*fpp, ECTF_BADID);
      return NULL;
    }

  /* Check for a type in the static portion.  */

  idx = LCTF_TYPE_TO_INDEX (fp, type);
  if (idx > 0 && (unsigned long) idx <= fp->ctf_typemax)
    {
      *fpp = fp;		/* Function returns ending CTF container.  */
      return (LCTF_INDEX_TO_TYPEPTR (fp, idx));
    }

  (void) ctf_set_errno (*fpp, ECTF_BADID);
  return NULL;
}

/* Given a symbol table index, return the info for the function described
   by the corresponding entry in the symbol table.  */

int
ctf_func_info (ctf_file_t *fp, unsigned long symidx, ctf_funcinfo_t *fip)
{
  const ctf_sect_t *sp = &fp->ctf_symtab;
  const uint32_t *dp;
  uint32_t info, kind, n;

  if (sp->cts_data == NULL)
    return (ctf_set_errno (fp, ECTF_NOSYMTAB));

  if (symidx >= fp->ctf_nsyms)
    return (ctf_set_errno (fp, EINVAL));

  if (sp->cts_entsize == sizeof (Elf32_Sym))
    {
      const Elf32_Sym *symp = (Elf32_Sym *) sp->cts_data + symidx;
      if (ELF32_ST_TYPE (symp->st_info) != STT_FUNC)
	return (ctf_set_errno (fp, ECTF_NOTFUNC));
    }
  else
    {
      const Elf64_Sym *symp = (Elf64_Sym *) sp->cts_data + symidx;
      if (ELF64_ST_TYPE (symp->st_info) != STT_FUNC)
	return (ctf_set_errno (fp, ECTF_NOTFUNC));
    }

  if (fp->ctf_sxlate[symidx] == -1u)
    return (ctf_set_errno (fp, ECTF_NOFUNCDAT));

  dp = (uint32_t *) ((uintptr_t) fp->ctf_buf + fp->ctf_sxlate[symidx]);

  info = *dp++;
  kind = LCTF_INFO_KIND (fp, info);
  n = LCTF_INFO_VLEN (fp, info);

  if (kind == CTF_K_UNKNOWN && n == 0)
    return (ctf_set_errno (fp, ECTF_NOFUNCDAT));

  if (kind != CTF_K_FUNCTION)
    return (ctf_set_errno (fp, ECTF_CORRUPT));

  fip->ctc_return = *dp++;
  fip->ctc_argc = n;
  fip->ctc_flags = 0;

  if (n != 0 && dp[n - 1] == 0)
    {
      fip->ctc_flags |= CTF_FUNC_VARARG;
      fip->ctc_argc--;
    }

  return 0;
}

/* Given a symbol table index, return the arguments for the function described
   by the corresponding entry in the symbol table.  */

int
ctf_func_args (ctf_file_t * fp, unsigned long symidx, uint32_t argc,
	       ctf_id_t * argv)
{
  const uint32_t *dp;
  ctf_funcinfo_t f;

  if (ctf_func_info (fp, symidx, &f) < 0)
    return -1;			/* errno is set for us.  */

  /* The argument data is two uint32_t's past the translation table
     offset: one for the function info, and one for the return type. */

  dp = (uint32_t *) ((uintptr_t) fp->ctf_buf + fp->ctf_sxlate[symidx]) + 2;

  for (argc = MIN (argc, f.ctc_argc); argc != 0; argc--)
    *argv++ = *dp++;

  return 0;
}
