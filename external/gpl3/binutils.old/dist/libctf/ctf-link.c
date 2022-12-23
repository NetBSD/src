/* CTF linking.
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

/* Type tracking machinery.  */

/* Record the correspondence between a source and ctf_add_type()-added
   destination type: both types are translated into parent type IDs if need be,
   so they relate to the actual container they are in.  Outside controlled
   circumstances (like linking) it is probably not useful to do more than
   compare these pointers, since there is nothing stopping the user closing the
   source container whenever they want to.

   Our OOM handling here is just to not do anything, because this is called deep
   enough in the call stack that doing anything useful is painfully difficult:
   the worst consequence if we do OOM is a bit of type duplication anyway.  */

void
ctf_add_type_mapping (ctf_file_t *src_fp, ctf_id_t src_type,
		      ctf_file_t *dst_fp, ctf_id_t dst_type)
{
  if (LCTF_TYPE_ISPARENT (src_fp, src_type) && src_fp->ctf_parent)
    src_fp = src_fp->ctf_parent;

  src_type = LCTF_TYPE_TO_INDEX(src_fp, src_type);

  if (LCTF_TYPE_ISPARENT (dst_fp, dst_type) && dst_fp->ctf_parent)
    dst_fp = dst_fp->ctf_parent;

  dst_type = LCTF_TYPE_TO_INDEX(dst_fp, dst_type);

  /* This dynhash is a bit tricky: it has a multivalued (structural) key, so we
     need to use the sized-hash machinery to generate key hashing and equality
     functions.  */

  if (dst_fp->ctf_link_type_mapping == NULL)
    {
      ctf_hash_fun f = ctf_hash_type_mapping_key;
      ctf_hash_eq_fun e = ctf_hash_eq_type_mapping_key;

      if ((dst_fp->ctf_link_type_mapping = ctf_dynhash_create (f, e, free,
							       NULL)) == NULL)
	return;
    }

  ctf_link_type_mapping_key_t *key;
  key = calloc (1, sizeof (struct ctf_link_type_mapping_key));
  if (!key)
    return;

  key->cltm_fp = src_fp;
  key->cltm_idx = src_type;

  ctf_dynhash_insert (dst_fp->ctf_link_type_mapping, key,
		      (void *) (uintptr_t) dst_type);
}

/* Look up a type mapping: return 0 if none.  The DST_FP is modified to point to
   the parent if need be.  The ID returned is from the dst_fp's perspective.  */
ctf_id_t
ctf_type_mapping (ctf_file_t *src_fp, ctf_id_t src_type, ctf_file_t **dst_fp)
{
  ctf_link_type_mapping_key_t key;
  ctf_file_t *target_fp = *dst_fp;
  ctf_id_t dst_type = 0;

  if (LCTF_TYPE_ISPARENT (src_fp, src_type) && src_fp->ctf_parent)
    src_fp = src_fp->ctf_parent;

  src_type = LCTF_TYPE_TO_INDEX(src_fp, src_type);
  key.cltm_fp = src_fp;
  key.cltm_idx = src_type;

  if (target_fp->ctf_link_type_mapping)
    dst_type = (uintptr_t) ctf_dynhash_lookup (target_fp->ctf_link_type_mapping,
					       &key);

  if (dst_type != 0)
    {
      dst_type = LCTF_INDEX_TO_TYPE (target_fp, dst_type,
				     target_fp->ctf_parent != NULL);
      *dst_fp = target_fp;
      return dst_type;
    }

  if (target_fp->ctf_parent)
    target_fp = target_fp->ctf_parent;
  else
    return 0;

  if (target_fp->ctf_link_type_mapping)
    dst_type = (uintptr_t) ctf_dynhash_lookup (target_fp->ctf_link_type_mapping,
					       &key);

  if (dst_type)
    dst_type = LCTF_INDEX_TO_TYPE (target_fp, dst_type,
				   target_fp->ctf_parent != NULL);

  *dst_fp = target_fp;
  return dst_type;
}

/* Linker machinery.

   CTF linking consists of adding CTF archives full of content to be merged into
   this one to the current file (which must be writable) by calling
   ctf_link_add_ctf().  Once this is done, a call to ctf_link() will merge the
   type tables together, generating new CTF files as needed, with this one as a
   parent, to contain types from the inputs which conflict.
   ctf_link_add_strtab() takes a callback which provides string/offset pairs to
   be added to the external symbol table and deduplicated from all CTF string
   tables in the output link; ctf_link_shuffle_syms() takes a callback which
   provides symtab entries in ascending order, and shuffles the function and
   data sections to match; and ctf_link_write() emits a CTF file (if there are
   no conflicts requiring per-compilation-unit sub-CTF files) or CTF archives
   (otherwise) and returns it, suitable for addition in the .ctf section of the
   output.  */

/* Add a file to a link.  */

static void ctf_arc_close_thunk (void *arc)
{
  ctf_arc_close ((ctf_archive_t *) arc);
}

static void ctf_file_close_thunk (void *file)
{
  ctf_file_close ((ctf_file_t *) file);
}

int
ctf_link_add_ctf (ctf_file_t *fp, ctf_archive_t *ctf, const char *name)
{
  char *dupname = NULL;

  if (fp->ctf_link_outputs)
    return (ctf_set_errno (fp, ECTF_LINKADDEDLATE));
  if (fp->ctf_link_inputs == NULL)
    fp->ctf_link_inputs = ctf_dynhash_create (ctf_hash_string,
					      ctf_hash_eq_string, free,
					      ctf_arc_close_thunk);

  if (fp->ctf_link_inputs == NULL)
    goto oom;

  if ((dupname = strdup (name)) == NULL)
    goto oom;

  if (ctf_dynhash_insert (fp->ctf_link_inputs, dupname, ctf) < 0)
    goto oom;

  return 0;
 oom:
  free (fp->ctf_link_inputs);
  fp->ctf_link_inputs = NULL;
  free (dupname);
  return (ctf_set_errno (fp, ENOMEM));
}

/* Return a per-CU output CTF dictionary suitable for the given CU, creating and
   interning it if need be.  */

static ctf_file_t *
ctf_create_per_cu (ctf_file_t *fp, const char *filename, const char *cuname)
{
  ctf_file_t *cu_fp;
  const char *ctf_name = NULL;
  char *dynname = NULL;

  /* First, check the mapping table and translate the per-CU name we use
     accordingly.  We check both the input filename and the CU name.  Only if
     neither are set do we fall back to the input filename as the per-CU
     dictionary name.  We prefer the filename because this is easier for likely
     callers to determine.  */

  if (fp->ctf_link_cu_mapping)
    {
      if (((ctf_name = ctf_dynhash_lookup (fp->ctf_link_cu_mapping, filename)) == NULL) &&
	  ((ctf_name = ctf_dynhash_lookup (fp->ctf_link_cu_mapping, cuname)) == NULL))
	ctf_name = filename;
    }

  if (ctf_name == NULL)
    ctf_name = filename;

  if ((cu_fp = ctf_dynhash_lookup (fp->ctf_link_outputs, ctf_name)) == NULL)
    {
      int err;

      if ((cu_fp = ctf_create (&err)) == NULL)
	{
	  ctf_dprintf ("Cannot create per-CU CTF archive for CU %s from "
		       "input file %s: %s\n", cuname, filename,
		       ctf_errmsg (err));
	  ctf_set_errno (fp, err);
	  return NULL;
	}

      if ((dynname = strdup (ctf_name)) == NULL)
	goto oom;
      if (ctf_dynhash_insert (fp->ctf_link_outputs, dynname, cu_fp) < 0)
	goto oom;

      ctf_import (cu_fp, fp);
      ctf_cuname_set (cu_fp, cuname);
      ctf_parent_name_set (cu_fp, _CTF_SECTION);
    }
  return cu_fp;

 oom:
  free (dynname);
  ctf_file_close (cu_fp);
  ctf_set_errno (fp, ENOMEM);
  return NULL;
}

/* Add a mapping directing that the CU named FROM should have its
   conflicting/non-duplicate types (depending on link mode) go into a container
   named TO.  Many FROMs can share a TO: in this case, the effect on conflicting
   types is not yet defined (but in time an auto-renaming algorithm will be
   added: ugly, but there is really no right thing one can do in this
   situation).

   We forcibly add a container named TO in every case, even though it may well
   wind up empty, because clients that use this facility usually expect to find
   every TO container present, even if empty, and malfunction otherwise.  */

int
ctf_link_add_cu_mapping (ctf_file_t *fp, const char *from, const char *to)
{
  int err;
  char *f, *t;

  if (fp->ctf_link_cu_mapping == NULL)
    fp->ctf_link_cu_mapping = ctf_dynhash_create (ctf_hash_string,
						  ctf_hash_eq_string, free,
						  free);
  if (fp->ctf_link_cu_mapping == NULL)
    return ctf_set_errno (fp, ENOMEM);

  if (fp->ctf_link_outputs == NULL)
    fp->ctf_link_outputs = ctf_dynhash_create (ctf_hash_string,
					       ctf_hash_eq_string, free,
					       ctf_file_close_thunk);

  if (fp->ctf_link_outputs == NULL)
    return ctf_set_errno (fp, ENOMEM);

  f = strdup (from);
  t = strdup (to);
  if (!f || !t)
    goto oom;

  if (ctf_create_per_cu (fp, t, t) == NULL)
    goto oom_noerrno;				/* Errno is set for us.  */

  err = ctf_dynhash_insert (fp->ctf_link_cu_mapping, f, t);
  if (err)
    {
      ctf_set_errno (fp, err);
      goto oom_noerrno;
    }

  return 0;

 oom:
  ctf_set_errno (fp, errno);
 oom_noerrno:
  free (f);
  free (t);
  return -1;
}

/* Set a function which is called to transform the names of archive members.
   This is useful for applying regular transformations to many names, where
   ctf_link_add_cu_mapping applies arbitrarily irregular changes to single
   names.  The member name changer is applied at ctf_link_write time, so it
   cannot conflate multiple CUs into one the way ctf_link_add_cu_mapping can.
   The changer function accepts a name and should return a new
   dynamically-allocated name, or NULL if the name should be left unchanged.  */
void
ctf_link_set_memb_name_changer (ctf_file_t *fp,
				ctf_link_memb_name_changer_f *changer,
				void *arg)
{
  fp->ctf_link_memb_name_changer = changer;
  fp->ctf_link_memb_name_changer_arg = arg;
}

typedef struct ctf_link_in_member_cb_arg
{
  ctf_file_t *out_fp;
  const char *file_name;
  ctf_file_t *in_fp;
  ctf_file_t *main_input_fp;
  const char *cu_name;
  char *arcname;
  int done_main_member;
  int share_mode;
  int in_input_cu_file;
} ctf_link_in_member_cb_arg_t;

/* Link one type into the link.  We rely on ctf_add_type() to detect
   duplicates.  This is not terribly reliable yet (unnmamed types will be
   mindlessly duplicated), but will improve shortly.  */

static int
ctf_link_one_type (ctf_id_t type, int isroot _libctf_unused_, void *arg_)
{
  ctf_link_in_member_cb_arg_t *arg = (ctf_link_in_member_cb_arg_t *) arg_;
  ctf_file_t *per_cu_out_fp;
  int err;

  if (arg->share_mode != CTF_LINK_SHARE_UNCONFLICTED)
    {
      ctf_dprintf ("Share-duplicated mode not yet implemented.\n");
      return ctf_set_errno (arg->out_fp, ECTF_NOTYET);
    }

  /* Simply call ctf_add_type: if it reports a conflict and we're adding to the
     main CTF file, add to the per-CU archive member instead, creating it if
     necessary.  If we got this type from a per-CU archive member, add it
     straight back to the corresponding member in the output.  */

  if (!arg->in_input_cu_file)
    {
      if (ctf_add_type (arg->out_fp, arg->in_fp, type) != CTF_ERR)
	return 0;

      err = ctf_errno (arg->out_fp);
      if (err != ECTF_CONFLICT)
	{
	  if (err != ECTF_NONREPRESENTABLE)
	    ctf_dprintf ("Cannot link type %lx from archive member %s, input file %s "
			 "into output link: %s\n", type, arg->arcname, arg->file_name,
			 ctf_errmsg (err));
	  /* We must ignore this problem or we end up losing future types, then
	     trying to link the variables in, then exploding.  Better to link as
	     much as possible.  XXX when we add a proper link warning
	     infrastructure, we should report the error here!  */
	  return 0;
	}
      ctf_set_errno (arg->out_fp, 0);
    }

  if ((per_cu_out_fp = ctf_create_per_cu (arg->out_fp, arg->file_name,
					  arg->cu_name)) == NULL)
    return -1;					/* Errno is set for us.  */

  if (ctf_add_type (per_cu_out_fp, arg->in_fp, type) != CTF_ERR)
    return 0;

  err = ctf_errno (per_cu_out_fp);
  if (err != ECTF_NONREPRESENTABLE)
    ctf_dprintf ("Cannot link type %lx from CTF archive member %s, input file %s "
		 "into output per-CU CTF archive member %s: %s: skipped\n", type,
		 arg->arcname, arg->file_name, arg->arcname,
		 ctf_errmsg (err));
  if (err == ECTF_CONFLICT)
      /* Conflicts are possible at this stage only if a non-ld user has combined
	 multiple TUs into a single output dictionary.  Even in this case we do not
	 want to stop the link or propagate the error.  */
      ctf_set_errno (arg->out_fp, 0);

  return 0;					/* As above: do not lose types.  */
}

/* Check if we can safely add a variable with the given type to this container.  */

static int
check_variable (const char *name, ctf_file_t *fp, ctf_id_t type,
		ctf_dvdef_t **out_dvd)
{
  ctf_dvdef_t *dvd;

  dvd = ctf_dynhash_lookup (fp->ctf_dvhash, name);
  *out_dvd = dvd;
  if (!dvd)
    return 1;

  if (dvd->dvd_type != type)
    {
      /* Variable here.  Wrong type: cannot add.  Just skip it, because there is
	 no way to express this in CTF.  (This might be the parent, in which
	 case we'll try adding in the child first, and only then give up.)  */
      ctf_dprintf ("Inexpressible duplicate variable %s skipped.\n", name);
    }

  return 0;				      /* Already exists.  */
}

/* Link one variable in.  */

static int
ctf_link_one_variable (const char *name, ctf_id_t type, void *arg_)
{
  ctf_link_in_member_cb_arg_t *arg = (ctf_link_in_member_cb_arg_t *) arg_;
  ctf_file_t *per_cu_out_fp;
  ctf_id_t dst_type = 0;
  ctf_file_t *check_fp;
  ctf_dvdef_t *dvd;

  /* In unconflicted link mode, if this type is mapped to a type in the parent
     container, we want to try to add to that first: if it reports a duplicate,
     or if the type is in a child already, add straight to the child.  */

  check_fp = arg->out_fp;

  dst_type = ctf_type_mapping (arg->in_fp, type, &check_fp);
  if (dst_type != 0)
    {
      if (check_fp == arg->out_fp)
	{
	  if (check_variable (name, check_fp, dst_type, &dvd))
	    {
	      /* No variable here: we can add it.  */
	      if (ctf_add_variable (check_fp, name, dst_type) < 0)
		return (ctf_set_errno (arg->out_fp, ctf_errno (check_fp)));
	      return 0;
	    }

	  /* Already present?  Nothing to do.  */
	  if (dvd && dvd->dvd_type == type)
	    return 0;
	}
    }

  /* Can't add to the parent due to a name clash, or because it references a
     type only present in the child.  Try adding to the child, creating if need
     be.  */

  if ((per_cu_out_fp = ctf_create_per_cu (arg->out_fp, arg->file_name,
					  arg->cu_name)) == NULL)
    return -1;					/* Errno is set for us.  */

  /* If the type was not found, check for it in the child too. */
  if (dst_type == 0)
    {
      check_fp = per_cu_out_fp;
      dst_type = ctf_type_mapping (arg->in_fp, type, &check_fp);

      if (dst_type == 0)
	{
	  ctf_dprintf ("Type %lx for variable %s in input file %s not "
		       "found: skipped.\n", type, name, arg->file_name);
	  /* Do not terminate the link: just skip the variable.  */
	  return 0;
	}
    }

  if (check_variable (name, per_cu_out_fp, dst_type, &dvd))
    if (ctf_add_variable (per_cu_out_fp, name, dst_type) < 0)
      return (ctf_set_errno (arg->out_fp, ctf_errno (per_cu_out_fp)));
  return 0;
}

/* Merge every type and variable in this archive member into the link, so we can
   relink things that have already had ld run on them.  We use the archive
   member name, sans any leading '.ctf.', as the CU name for ambiguous types if
   there is one and it's not the default: otherwise, we use the name of the
   input file.  */
static int
ctf_link_one_input_archive_member (ctf_file_t *in_fp, const char *name, void *arg_)
{
  ctf_link_in_member_cb_arg_t *arg = (ctf_link_in_member_cb_arg_t *) arg_;
  int err = 0;

  if (strcmp (name, _CTF_SECTION) == 0)
    {
      /* This file is the default member of this archive, and has already been
	 explicitly processed.

	 In the default sharing mode of CTF_LINK_SHARE_UNCONFLICTED, it does no
	 harm to rescan an existing shared repo again: all the types will just
	 end up in the same place.  But in CTF_LINK_SHARE_DUPLICATED mode, this
	 causes the system to erroneously conclude that all types are duplicated
	 and should be shared, even if they are not.  */

      if (arg->done_main_member)
	return 0;
      arg->arcname = strdup (".ctf.");
      if (arg->arcname)
	{
	  char *new_name;

	  new_name = ctf_str_append (arg->arcname, arg->file_name);
	  if (new_name)
	    arg->arcname = new_name;
	  else
	    free (arg->arcname);
	}
    }
  else
    {
      arg->arcname = strdup (name);

      /* Get ambiguous types from our parent.  */
      ctf_import (in_fp, arg->main_input_fp);
      arg->in_input_cu_file = 1;
    }

  if (!arg->arcname)
    return ctf_set_errno (in_fp, ENOMEM);

  arg->cu_name = name;
  if (strncmp (arg->cu_name, ".ctf.", strlen (".ctf.")) == 0)
    arg->cu_name += strlen (".ctf.");
  arg->in_fp = in_fp;

  if ((err = ctf_type_iter_all (in_fp, ctf_link_one_type, arg)) > -1)
    err = ctf_variable_iter (in_fp, ctf_link_one_variable, arg);

  arg->in_input_cu_file = 0;
  free (arg->arcname);

  if (err < 0)
      return -1;				/* Errno is set for us.  */

  return 0;
}

/* Dump the unnecessary link type mapping after one input file is processed.  */
static void
empty_link_type_mapping (void *key _libctf_unused_, void *value,
			 void *arg _libctf_unused_)
{
  ctf_file_t *fp = (ctf_file_t *) value;

  if (fp->ctf_link_type_mapping)
    ctf_dynhash_empty (fp->ctf_link_type_mapping);
}

/* Link one input file's types into the output file.  */
static void
ctf_link_one_input_archive (void *key, void *value, void *arg_)
{
  const char *file_name = (const char *) key;
  ctf_archive_t *arc = (ctf_archive_t *) value;
  ctf_link_in_member_cb_arg_t *arg = (ctf_link_in_member_cb_arg_t *) arg_;
  int err;

  arg->file_name = file_name;
  arg->done_main_member = 0;
  if ((arg->main_input_fp = ctf_arc_open_by_name (arc, NULL, &err)) == NULL)
    if (err != ECTF_ARNNAME)
      {
	ctf_dprintf ("Cannot open main archive member in input file %s in the "
		     "link: skipping: %s.\n", arg->file_name,
		     ctf_errmsg (err));
	return;
      }

  if (ctf_link_one_input_archive_member (arg->main_input_fp,
					 _CTF_SECTION, arg) < 0)
    {
      ctf_file_close (arg->main_input_fp);
      return;
    }
  arg->done_main_member = 1;
  if (ctf_archive_iter (arc, ctf_link_one_input_archive_member, arg) < 0)
    ctf_dprintf ("Cannot traverse archive in input file %s: link "
		 "cannot continue: %s.\n", arg->file_name,
		 ctf_errmsg (ctf_errno (arg->out_fp)));
  else
    {
      /* The only error indication to the caller is the errno: so ensure that it
	 is zero if there was no actual error from the caller.  */
      ctf_set_errno (arg->out_fp, 0);
    }
  ctf_file_close (arg->main_input_fp);

  /* Discard the now-unnecessary mapping table data.  */
  if (arg->out_fp->ctf_link_type_mapping)
    ctf_dynhash_empty (arg->out_fp->ctf_link_type_mapping);
  ctf_dynhash_iter (arg->out_fp->ctf_link_outputs, empty_link_type_mapping, NULL);
}

/* Merge types and variable sections in all files added to the link
   together.  */
int
ctf_link (ctf_file_t *fp, int share_mode)
{
  ctf_link_in_member_cb_arg_t arg;

  memset (&arg, 0, sizeof (struct ctf_link_in_member_cb_arg));
  arg.out_fp = fp;
  arg.share_mode = share_mode;

  if (fp->ctf_link_inputs == NULL)
    return 0;					/* Nothing to do. */

  if (fp->ctf_link_outputs == NULL)
    fp->ctf_link_outputs = ctf_dynhash_create (ctf_hash_string,
					       ctf_hash_eq_string, free,
					       ctf_file_close_thunk);

  if (fp->ctf_link_outputs == NULL)
    return ctf_set_errno (fp, ENOMEM);

  ctf_dynhash_iter (fp->ctf_link_inputs, ctf_link_one_input_archive,
		    &arg);

  if (ctf_errno (fp) != 0)
    return -1;
  return 0;
}

typedef struct ctf_link_out_string_cb_arg
{
  const char *str;
  uint32_t offset;
  int err;
} ctf_link_out_string_cb_arg_t;

/* Intern a string in the string table of an output per-CU CTF file.  */
static void
ctf_link_intern_extern_string (void *key _libctf_unused_, void *value,
			       void *arg_)
{
  ctf_file_t *fp = (ctf_file_t *) value;
  ctf_link_out_string_cb_arg_t *arg = (ctf_link_out_string_cb_arg_t *) arg_;

  fp->ctf_flags |= LCTF_DIRTY;
  if (!ctf_str_add_external (fp, arg->str, arg->offset))
    arg->err = ENOMEM;
}

/* Repeatedly call ADD_STRING to acquire strings from the external string table,
   adding them to the atoms table for this CU and all subsidiary CUs.

   If ctf_link() is also called, it must be called first if you want the new CTF
   files ctf_link() can create to get their strings dedupped against the ELF
   strtab properly.  */
int
ctf_link_add_strtab (ctf_file_t *fp, ctf_link_strtab_string_f *add_string,
		     void *arg)
{
  const char *str;
  uint32_t offset;
  int err = 0;

  while ((str = add_string (&offset, arg)) != NULL)
    {
      ctf_link_out_string_cb_arg_t iter_arg = { str, offset, 0 };

      fp->ctf_flags |= LCTF_DIRTY;
      if (!ctf_str_add_external (fp, str, offset))
	err = ENOMEM;

      ctf_dynhash_iter (fp->ctf_link_outputs, ctf_link_intern_extern_string,
			&iter_arg);
      if (iter_arg.err)
	err = iter_arg.err;
    }

  return -err;
}

/* Not yet implemented.  */
int
ctf_link_shuffle_syms (ctf_file_t *fp _libctf_unused_,
		       ctf_link_iter_symbol_f *add_sym _libctf_unused_,
		       void *arg _libctf_unused_)
{
  return 0;
}

typedef struct ctf_name_list_accum_cb_arg
{
  char **names;
  ctf_file_t *fp;
  ctf_file_t **files;
  size_t i;
  char **dynames;
  size_t ndynames;
} ctf_name_list_accum_cb_arg_t;

/* Accumulate the names and a count of the names in the link output hash.  */
static void
ctf_accumulate_archive_names (void *key, void *value, void *arg_)
{
  const char *name = (const char *) key;
  ctf_file_t *fp = (ctf_file_t *) value;
  char **names;
  ctf_file_t **files;
  ctf_name_list_accum_cb_arg_t *arg = (ctf_name_list_accum_cb_arg_t *) arg_;

  if ((names = realloc (arg->names, sizeof (char *) * ++(arg->i))) == NULL)
    {
      (arg->i)--;
      ctf_set_errno (arg->fp, ENOMEM);
      return;
    }

  if ((files = realloc (arg->files, sizeof (ctf_file_t *) * arg->i)) == NULL)
    {
      (arg->i)--;
      ctf_set_errno (arg->fp, ENOMEM);
      return;
    }

  /* Allow the caller to get in and modify the name at the last minute.  If the
     caller *does* modify the name, we have to stash away the new name the
     caller returned so we can free it later on.  (The original name is the key
     of the ctf_link_outputs hash and is freed by the dynhash machinery.)  */

  if (fp->ctf_link_memb_name_changer)
    {
      char **dynames;
      char *dyname;
      void *nc_arg = fp->ctf_link_memb_name_changer_arg;

      dyname = fp->ctf_link_memb_name_changer (fp, name, nc_arg);

      if (dyname != NULL)
	{
	  if ((dynames = realloc (arg->dynames,
				  sizeof (char *) * ++(arg->ndynames))) == NULL)
	    {
	      (arg->ndynames)--;
	      ctf_set_errno (arg->fp, ENOMEM);
	      return;
	    }
	    arg->dynames = dynames;
	    name = (const char *) dyname;
	}
    }

  arg->names = names;
  arg->names[(arg->i) - 1] = (char *) name;
  arg->files = files;
  arg->files[(arg->i) - 1] = fp;
}

/* Change the name of the parent CTF section, if the name transformer has got to
   it.  */
static void
ctf_change_parent_name (void *key _libctf_unused_, void *value, void *arg)
{
  ctf_file_t *fp = (ctf_file_t *) value;
  const char *name = (const char *) arg;

  ctf_parent_name_set (fp, name);
}

/* Write out a CTF archive (if there are per-CU CTF files) or a CTF file
   (otherwise) into a new dynamically-allocated string, and return it.
   Members with sizes above THRESHOLD are compressed.  */
unsigned char *
ctf_link_write (ctf_file_t *fp, size_t *size, size_t threshold)
{
  ctf_name_list_accum_cb_arg_t arg;
  char **names;
  char *transformed_name = NULL;
  ctf_file_t **files;
  FILE *f = NULL;
  int err;
  long fsize;
  const char *errloc;
  unsigned char *buf = NULL;

  memset (&arg, 0, sizeof (ctf_name_list_accum_cb_arg_t));
  arg.fp = fp;

  if (fp->ctf_link_outputs)
    {
      ctf_dynhash_iter (fp->ctf_link_outputs, ctf_accumulate_archive_names, &arg);
      if (ctf_errno (fp) < 0)
	{
	  errloc = "hash creation";
	  goto err;
	}
    }

  /* No extra outputs? Just write a simple ctf_file_t.  */
  if (arg.i == 0)
    return ctf_write_mem (fp, size, threshold);

  /* Writing an archive.  Stick ourselves (the shared repository, parent of all
     other archives) on the front of it with the default name.  */
  if ((names = realloc (arg.names, sizeof (char *) * (arg.i + 1))) == NULL)
    {
      errloc = "name reallocation";
      goto err_no;
    }
  arg.names = names;
  memmove (&(arg.names[1]), arg.names, sizeof (char *) * (arg.i));

  arg.names[0] = (char *) _CTF_SECTION;
  if (fp->ctf_link_memb_name_changer)
    {
      void *nc_arg = fp->ctf_link_memb_name_changer_arg;

      transformed_name = fp->ctf_link_memb_name_changer (fp, _CTF_SECTION,
							 nc_arg);

      if (transformed_name != NULL)
	{
	  arg.names[0] = transformed_name;
	  ctf_dynhash_iter (fp->ctf_link_outputs, ctf_change_parent_name,
			    transformed_name);
	}
    }

  if ((files = realloc (arg.files,
			sizeof (struct ctf_file *) * (arg.i + 1))) == NULL)
    {
      errloc = "ctf_file reallocation";
      goto err_no;
    }
  arg.files = files;
  memmove (&(arg.files[1]), arg.files, sizeof (ctf_file_t *) * (arg.i));
  arg.files[0] = fp;

  if ((f = tmpfile ()) == NULL)
    {
      errloc = "tempfile creation";
      goto err_no;
    }

  if ((err = ctf_arc_write_fd (fileno (f), arg.files, arg.i + 1,
			       (const char **) arg.names,
			       threshold)) < 0)
    {
      errloc = "archive writing";
      ctf_set_errno (fp, err);
      goto err;
    }

  if (fseek (f, 0, SEEK_END) < 0)
    {
      errloc = "seeking to end";
      goto err_no;
    }

  if ((fsize = ftell (f)) < 0)
    {
      errloc = "filesize determination";
      goto err_no;
    }

  if (fseek (f, 0, SEEK_SET) < 0)
    {
      errloc = "filepos resetting";
      goto err_no;
    }

  if ((buf = malloc (fsize)) == NULL)
    {
      errloc = "CTF archive buffer allocation";
      goto err_no;
    }

  while (!feof (f) && fread (buf, fsize, 1, f) == 0)
    if (ferror (f))
      {
	errloc = "reading archive from temporary file";
	goto err_no;
      }

  *size = fsize;
  free (arg.names);
  free (arg.files);
  free (transformed_name);
  if (arg.ndynames)
    {
      size_t i;
      for (i = 0; i < arg.ndynames; i++)
	free (arg.dynames[i]);
      free (arg.dynames);
    }
  return buf;

 err_no:
  ctf_set_errno (fp, errno);
 err:
  free (buf);
  if (f)
    fclose (f);
  free (arg.names);
  free (arg.files);
  free (transformed_name);
  if (arg.ndynames)
    {
      size_t i;
      for (i = 0; i < arg.ndynames; i++)
	free (arg.dynames[i]);
      free (arg.dynames);
    }
  ctf_dprintf ("Cannot write archive in link: %s failure: %s\n", errloc,
	       ctf_errmsg (ctf_errno (fp)));
  return NULL;
}
