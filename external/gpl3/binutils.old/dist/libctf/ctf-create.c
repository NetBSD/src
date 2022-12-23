/* CTF file creation.
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
#include <sys/param.h>
#include <assert.h>
#include <string.h>
#include <zlib.h>

#ifndef roundup
#define roundup(x, y)  ((((x) + ((y) - 1)) / (y)) * (y))
#endif

/* Make sure the ptrtab has enough space for at least one more type.

   We start with 4KiB of ptrtab, enough for a thousand types, then grow it 25%
   at a time.  */

static int
ctf_grow_ptrtab (ctf_file_t *fp)
{
  size_t new_ptrtab_len = fp->ctf_ptrtab_len;

  /* We allocate one more ptrtab entry than we need, for the initial zero,
     plus one because the caller will probably allocate a new type.  */

  if (fp->ctf_ptrtab == NULL)
    new_ptrtab_len = 1024;
  else if ((fp->ctf_typemax + 2) > fp->ctf_ptrtab_len)
    new_ptrtab_len = fp->ctf_ptrtab_len * 1.25;

  if (new_ptrtab_len != fp->ctf_ptrtab_len)
    {
      uint32_t *new_ptrtab;

      if ((new_ptrtab = realloc (fp->ctf_ptrtab,
				 new_ptrtab_len * sizeof (uint32_t))) == NULL)
	return (ctf_set_errno (fp, ENOMEM));

      fp->ctf_ptrtab = new_ptrtab;
      memset (fp->ctf_ptrtab + fp->ctf_ptrtab_len, 0,
	      (new_ptrtab_len - fp->ctf_ptrtab_len) * sizeof (uint32_t));
      fp->ctf_ptrtab_len = new_ptrtab_len;
    }
  return 0;
}

/* To create an empty CTF container, we just declare a zeroed header and call
   ctf_bufopen() on it.  If ctf_bufopen succeeds, we mark the new container r/w
   and initialize the dynamic members.  We start assigning type IDs at 1 because
   type ID 0 is used as a sentinel and a not-found indicator.  */

ctf_file_t *
ctf_create (int *errp)
{
  static const ctf_header_t hdr = { .cth_preamble = { CTF_MAGIC, CTF_VERSION, 0 } };

  ctf_dynhash_t *dthash;
  ctf_dynhash_t *dvhash;
  ctf_dynhash_t *structs = NULL, *unions = NULL, *enums = NULL, *names = NULL;
  ctf_sect_t cts;
  ctf_file_t *fp;

  libctf_init_debug();
  dthash = ctf_dynhash_create (ctf_hash_integer, ctf_hash_eq_integer,
			       NULL, NULL);
  if (dthash == NULL)
    {
      ctf_set_open_errno (errp, EAGAIN);
      goto err;
    }

  dvhash = ctf_dynhash_create (ctf_hash_string, ctf_hash_eq_string,
			       NULL, NULL);
  if (dvhash == NULL)
    {
      ctf_set_open_errno (errp, EAGAIN);
      goto err_dt;
    }

  structs = ctf_dynhash_create (ctf_hash_string, ctf_hash_eq_string,
				NULL, NULL);
  unions = ctf_dynhash_create (ctf_hash_string, ctf_hash_eq_string,
			       NULL, NULL);
  enums = ctf_dynhash_create (ctf_hash_string, ctf_hash_eq_string,
			      NULL, NULL);
  names = ctf_dynhash_create (ctf_hash_string, ctf_hash_eq_string,
			      NULL, NULL);
  if (!structs || !unions || !enums || !names)
    {
      ctf_set_open_errno (errp, EAGAIN);
      goto err_dv;
    }

  cts.cts_name = _CTF_SECTION;
  cts.cts_data = &hdr;
  cts.cts_size = sizeof (hdr);
  cts.cts_entsize = 1;

  if ((fp = ctf_bufopen_internal (&cts, NULL, NULL, NULL, 1, errp)) == NULL)
    goto err_dv;

  fp->ctf_structs.ctn_writable = structs;
  fp->ctf_unions.ctn_writable = unions;
  fp->ctf_enums.ctn_writable = enums;
  fp->ctf_names.ctn_writable = names;
  fp->ctf_dthash = dthash;
  fp->ctf_dvhash = dvhash;
  fp->ctf_dtoldid = 0;
  fp->ctf_snapshots = 1;
  fp->ctf_snapshot_lu = 0;

  ctf_set_ctl_hashes (fp);
  ctf_setmodel (fp, CTF_MODEL_NATIVE);
  if (ctf_grow_ptrtab (fp) < 0)
    {
      ctf_set_open_errno (errp, ctf_errno (fp));
      ctf_file_close (fp);
      return NULL;
    }

  return fp;

 err_dv:
  ctf_dynhash_destroy (structs);
  ctf_dynhash_destroy (unions);
  ctf_dynhash_destroy (enums);
  ctf_dynhash_destroy (names);
  ctf_dynhash_destroy (dvhash);
 err_dt:
  ctf_dynhash_destroy (dthash);
 err:
  return NULL;
}

static unsigned char *
ctf_copy_smembers (ctf_file_t *fp, ctf_dtdef_t *dtd, unsigned char *t)
{
  ctf_dmdef_t *dmd = ctf_list_next (&dtd->dtd_u.dtu_members);
  ctf_member_t ctm;

  for (; dmd != NULL; dmd = ctf_list_next (dmd))
    {
      ctf_member_t *copied;

      ctm.ctm_name = 0;
      ctm.ctm_type = (uint32_t) dmd->dmd_type;
      ctm.ctm_offset = (uint32_t) dmd->dmd_offset;

      memcpy (t, &ctm, sizeof (ctm));
      copied = (ctf_member_t *) t;
      if (dmd->dmd_name)
	ctf_str_add_ref (fp, dmd->dmd_name, &copied->ctm_name);

      t += sizeof (ctm);
    }

  return t;
}

static unsigned char *
ctf_copy_lmembers (ctf_file_t *fp, ctf_dtdef_t *dtd, unsigned char *t)
{
  ctf_dmdef_t *dmd = ctf_list_next (&dtd->dtd_u.dtu_members);
  ctf_lmember_t ctlm;

  for (; dmd != NULL; dmd = ctf_list_next (dmd))
    {
      ctf_lmember_t *copied;

      ctlm.ctlm_name = 0;
      ctlm.ctlm_type = (uint32_t) dmd->dmd_type;
      ctlm.ctlm_offsethi = CTF_OFFSET_TO_LMEMHI (dmd->dmd_offset);
      ctlm.ctlm_offsetlo = CTF_OFFSET_TO_LMEMLO (dmd->dmd_offset);

      memcpy (t, &ctlm, sizeof (ctlm));
      copied = (ctf_lmember_t *) t;
      if (dmd->dmd_name)
	ctf_str_add_ref (fp, dmd->dmd_name, &copied->ctlm_name);

      t += sizeof (ctlm);
    }

  return t;
}

static unsigned char *
ctf_copy_emembers (ctf_file_t *fp, ctf_dtdef_t *dtd, unsigned char *t)
{
  ctf_dmdef_t *dmd = ctf_list_next (&dtd->dtd_u.dtu_members);
  ctf_enum_t cte;

  for (; dmd != NULL; dmd = ctf_list_next (dmd))
    {
      ctf_enum_t *copied;

      cte.cte_value = dmd->dmd_value;
      memcpy (t, &cte, sizeof (cte));
      copied = (ctf_enum_t *) t;
      ctf_str_add_ref (fp, dmd->dmd_name, &copied->cte_name);
      t += sizeof (cte);
    }

  return t;
}

/* Sort a newly-constructed static variable array.  */

typedef struct ctf_sort_var_arg_cb
{
  ctf_file_t *fp;
  ctf_strs_t *strtab;
} ctf_sort_var_arg_cb_t;

static int
ctf_sort_var (const void *one_, const void *two_, void *arg_)
{
  const ctf_varent_t *one = one_;
  const ctf_varent_t *two = two_;
  ctf_sort_var_arg_cb_t *arg = arg_;

  return (strcmp (ctf_strraw_explicit (arg->fp, one->ctv_name, arg->strtab),
		  ctf_strraw_explicit (arg->fp, two->ctv_name, arg->strtab)));
}

/* Compatibility: just update the threshold for ctf_discard.  */
int
ctf_update (ctf_file_t *fp)
{
  if (!(fp->ctf_flags & LCTF_RDWR))
    return (ctf_set_errno (fp, ECTF_RDONLY));

  fp->ctf_dtoldid = fp->ctf_typemax;
  return 0;
}

/* If the specified CTF container is writable and has been modified, reload this
   container with the updated type definitions, ready for serialization.  In
   order to make this code and the rest of libctf as simple as possible, we
   perform updates by taking the dynamic type definitions and creating an
   in-memory CTF file containing the definitions, and then call
   ctf_simple_open_internal() on it.  We perform one extra trick here for the
   benefit of callers and to keep our code simple: ctf_simple_open_internal()
   will return a new ctf_file_t, but we want to keep the fp constant for the
   caller, so after ctf_simple_open_internal() returns, we use memcpy to swap
   the interior of the old and new ctf_file_t's, and then free the old.  */
int
ctf_serialize (ctf_file_t *fp)
{
  ctf_file_t ofp, *nfp;
  ctf_header_t hdr, *hdrp;
  ctf_dtdef_t *dtd;
  ctf_dvdef_t *dvd;
  ctf_varent_t *dvarents;
  ctf_strs_writable_t strtab;

  unsigned char *t;
  unsigned long i;
  size_t buf_size, type_size, nvars;
  unsigned char *buf, *newbuf;
  int err;

  if (!(fp->ctf_flags & LCTF_RDWR))
    return (ctf_set_errno (fp, ECTF_RDONLY));

  /* Update required?  */
  if (!(fp->ctf_flags & LCTF_DIRTY))
    return 0;

  /* Fill in an initial CTF header.  We will leave the label, object,
     and function sections empty and only output a header, type section,
     and string table.  The type section begins at a 4-byte aligned
     boundary past the CTF header itself (at relative offset zero).  */

  memset (&hdr, 0, sizeof (hdr));
  hdr.cth_magic = CTF_MAGIC;
  hdr.cth_version = CTF_VERSION;

  /* Iterate through the dynamic type definition list and compute the
     size of the CTF type section we will need to generate.  */

  for (type_size = 0, dtd = ctf_list_next (&fp->ctf_dtdefs);
       dtd != NULL; dtd = ctf_list_next (dtd))
    {
      uint32_t kind = LCTF_INFO_KIND (fp, dtd->dtd_data.ctt_info);
      uint32_t vlen = LCTF_INFO_VLEN (fp, dtd->dtd_data.ctt_info);

      if (dtd->dtd_data.ctt_size != CTF_LSIZE_SENT)
	type_size += sizeof (ctf_stype_t);
      else
	type_size += sizeof (ctf_type_t);

      switch (kind)
	{
	case CTF_K_INTEGER:
	case CTF_K_FLOAT:
	  type_size += sizeof (uint32_t);
	  break;
	case CTF_K_ARRAY:
	  type_size += sizeof (ctf_array_t);
	  break;
	case CTF_K_SLICE:
	  type_size += sizeof (ctf_slice_t);
	  break;
	case CTF_K_FUNCTION:
	  type_size += sizeof (uint32_t) * (vlen + (vlen & 1));
	  break;
	case CTF_K_STRUCT:
	case CTF_K_UNION:
	  if (dtd->dtd_data.ctt_size < CTF_LSTRUCT_THRESH)
	    type_size += sizeof (ctf_member_t) * vlen;
	  else
	    type_size += sizeof (ctf_lmember_t) * vlen;
	  break;
	case CTF_K_ENUM:
	  type_size += sizeof (ctf_enum_t) * vlen;
	  break;
	}
    }

  /* Computing the number of entries in the CTF variable section is much
     simpler.  */

  for (nvars = 0, dvd = ctf_list_next (&fp->ctf_dvdefs);
       dvd != NULL; dvd = ctf_list_next (dvd), nvars++);

  /* Compute the size of the CTF buffer we need, sans only the string table,
     then allocate a new buffer and memcpy the finished header to the start of
     the buffer.  (We will adjust this later with strtab length info.)  */

  hdr.cth_typeoff = hdr.cth_varoff + (nvars * sizeof (ctf_varent_t));
  hdr.cth_stroff = hdr.cth_typeoff + type_size;
  hdr.cth_strlen = 0;

  buf_size = sizeof (ctf_header_t) + hdr.cth_stroff + hdr.cth_strlen;

  if ((buf = malloc (buf_size)) == NULL)
    return (ctf_set_errno (fp, EAGAIN));

  memcpy (buf, &hdr, sizeof (ctf_header_t));
  t = (unsigned char *) buf + sizeof (ctf_header_t) + hdr.cth_varoff;

  hdrp = (ctf_header_t *) buf;
  if ((fp->ctf_flags & LCTF_CHILD) && (fp->ctf_parname != NULL))
    ctf_str_add_ref (fp, fp->ctf_parname, &hdrp->cth_parname);
  if (fp->ctf_cuname != NULL)
    ctf_str_add_ref (fp, fp->ctf_cuname, &hdrp->cth_cuname);

  /* Work over the variable list, translating everything into ctf_varent_t's and
     prepping the string table.  */

  dvarents = (ctf_varent_t *) t;
  for (i = 0, dvd = ctf_list_next (&fp->ctf_dvdefs); dvd != NULL;
       dvd = ctf_list_next (dvd), i++)
    {
      ctf_varent_t *var = &dvarents[i];

      ctf_str_add_ref (fp, dvd->dvd_name, &var->ctv_name);
      var->ctv_type = dvd->dvd_type;
    }
  assert (i == nvars);

  t += sizeof (ctf_varent_t) * nvars;

  assert (t == (unsigned char *) buf + sizeof (ctf_header_t) + hdr.cth_typeoff);

  /* We now take a final lap through the dynamic type definition list and copy
     the appropriate type records to the output buffer, noting down the
     strings as we go.  */

  for (dtd = ctf_list_next (&fp->ctf_dtdefs);
       dtd != NULL; dtd = ctf_list_next (dtd))
    {
      uint32_t kind = LCTF_INFO_KIND (fp, dtd->dtd_data.ctt_info);
      uint32_t vlen = LCTF_INFO_VLEN (fp, dtd->dtd_data.ctt_info);

      ctf_array_t cta;
      uint32_t encoding;
      size_t len;
      ctf_stype_t *copied;
      const char *name;

      if (dtd->dtd_data.ctt_size != CTF_LSIZE_SENT)
	len = sizeof (ctf_stype_t);
      else
	len = sizeof (ctf_type_t);

      memcpy (t, &dtd->dtd_data, len);
      copied = (ctf_stype_t *) t;  /* name is at the start: constant offset.  */
      if (copied->ctt_name
	  && (name = ctf_strraw (fp, copied->ctt_name)) != NULL)
	ctf_str_add_ref (fp, name, &copied->ctt_name);
      t += len;

      switch (kind)
	{
	case CTF_K_INTEGER:
	case CTF_K_FLOAT:
	  if (kind == CTF_K_INTEGER)
	    {
	      encoding = CTF_INT_DATA (dtd->dtd_u.dtu_enc.cte_format,
				       dtd->dtd_u.dtu_enc.cte_offset,
				       dtd->dtd_u.dtu_enc.cte_bits);
	    }
	  else
	    {
	      encoding = CTF_FP_DATA (dtd->dtd_u.dtu_enc.cte_format,
				      dtd->dtd_u.dtu_enc.cte_offset,
				      dtd->dtd_u.dtu_enc.cte_bits);
	    }
	  memcpy (t, &encoding, sizeof (encoding));
	  t += sizeof (encoding);
	  break;

	case CTF_K_SLICE:
	  memcpy (t, &dtd->dtd_u.dtu_slice, sizeof (struct ctf_slice));
	  t += sizeof (struct ctf_slice);
	  break;

	case CTF_K_ARRAY:
	  cta.cta_contents = (uint32_t) dtd->dtd_u.dtu_arr.ctr_contents;
	  cta.cta_index = (uint32_t) dtd->dtd_u.dtu_arr.ctr_index;
	  cta.cta_nelems = dtd->dtd_u.dtu_arr.ctr_nelems;
	  memcpy (t, &cta, sizeof (cta));
	  t += sizeof (cta);
	  break;

	case CTF_K_FUNCTION:
	  {
	    uint32_t *argv = (uint32_t *) (uintptr_t) t;
	    uint32_t argc;

	    for (argc = 0; argc < vlen; argc++)
	      *argv++ = (uint32_t) dtd->dtd_u.dtu_argv[argc];

	    if (vlen & 1)
	      *argv++ = 0;	/* Pad to 4-byte boundary.  */

	    t = (unsigned char *) argv;
	    break;
	  }

	case CTF_K_STRUCT:
	case CTF_K_UNION:
	  if (dtd->dtd_data.ctt_size < CTF_LSTRUCT_THRESH)
	    t = ctf_copy_smembers (fp, dtd, t);
	  else
	    t = ctf_copy_lmembers (fp, dtd, t);
	  break;

	case CTF_K_ENUM:
	  t = ctf_copy_emembers (fp, dtd, t);
	  break;
	}
    }
  assert (t == (unsigned char *) buf + sizeof (ctf_header_t) + hdr.cth_stroff);

  /* Construct the final string table and fill out all the string refs with the
     final offsets.  Then purge the refs list, because we're about to move this
     strtab onto the end of the buf, invalidating all the offsets.  */
  strtab = ctf_str_write_strtab (fp);
  ctf_str_purge_refs (fp);

  if (strtab.cts_strs == NULL)
    {
      free (buf);
      return (ctf_set_errno (fp, EAGAIN));
    }

  /* Now the string table is constructed, we can sort the buffer of
     ctf_varent_t's.  */
  ctf_sort_var_arg_cb_t sort_var_arg = { fp, (ctf_strs_t *) &strtab };
  ctf_qsort_r (dvarents, nvars, sizeof (ctf_varent_t), ctf_sort_var,
	       &sort_var_arg);

  if ((newbuf = ctf_realloc (fp, buf, buf_size + strtab.cts_len)) == NULL)
    {
      free (buf);
      free (strtab.cts_strs);
      return (ctf_set_errno (fp, EAGAIN));
    }
  buf = newbuf;
  memcpy (buf + buf_size, strtab.cts_strs, strtab.cts_len);
  hdrp = (ctf_header_t *) buf;
  hdrp->cth_strlen = strtab.cts_len;
  buf_size += hdrp->cth_strlen;
  free (strtab.cts_strs);

  /* Finally, we are ready to ctf_simple_open() the new container.  If this
     is successful, we then switch nfp and fp and free the old container.  */

  if ((nfp = ctf_simple_open_internal ((char *) buf, buf_size, NULL, 0,
				       0, NULL, 0, fp->ctf_syn_ext_strtab,
				       1, &err)) == NULL)
    {
      free (buf);
      return (ctf_set_errno (fp, err));
    }

  (void) ctf_setmodel (nfp, ctf_getmodel (fp));
  (void) ctf_import (nfp, fp->ctf_parent);

  nfp->ctf_refcnt = fp->ctf_refcnt;
  nfp->ctf_flags |= fp->ctf_flags & ~LCTF_DIRTY;
  if (nfp->ctf_dynbase == NULL)
    nfp->ctf_dynbase = buf;		/* Make sure buf is freed on close.  */
  nfp->ctf_dthash = fp->ctf_dthash;
  nfp->ctf_dtdefs = fp->ctf_dtdefs;
  nfp->ctf_dvhash = fp->ctf_dvhash;
  nfp->ctf_dvdefs = fp->ctf_dvdefs;
  nfp->ctf_dtoldid = fp->ctf_dtoldid;
  nfp->ctf_add_processing = fp->ctf_add_processing;
  nfp->ctf_snapshots = fp->ctf_snapshots + 1;
  nfp->ctf_specific = fp->ctf_specific;
  nfp->ctf_ptrtab = fp->ctf_ptrtab;
  nfp->ctf_ptrtab_len = fp->ctf_ptrtab_len;
  nfp->ctf_link_inputs = fp->ctf_link_inputs;
  nfp->ctf_link_outputs = fp->ctf_link_outputs;
  nfp->ctf_str_prov_offset = fp->ctf_str_prov_offset;
  nfp->ctf_syn_ext_strtab = fp->ctf_syn_ext_strtab;
  nfp->ctf_link_cu_mapping = fp->ctf_link_cu_mapping;
  nfp->ctf_link_type_mapping = fp->ctf_link_type_mapping;
  nfp->ctf_link_memb_name_changer = fp->ctf_link_memb_name_changer;
  nfp->ctf_link_memb_name_changer_arg = fp->ctf_link_memb_name_changer_arg;

  nfp->ctf_snapshot_lu = fp->ctf_snapshots;

  memcpy (&nfp->ctf_lookups, fp->ctf_lookups, sizeof (fp->ctf_lookups));
  nfp->ctf_structs = fp->ctf_structs;
  nfp->ctf_unions = fp->ctf_unions;
  nfp->ctf_enums = fp->ctf_enums;
  nfp->ctf_names = fp->ctf_names;

  fp->ctf_dthash = NULL;
  ctf_str_free_atoms (nfp);
  nfp->ctf_str_atoms = fp->ctf_str_atoms;
  nfp->ctf_prov_strtab = fp->ctf_prov_strtab;
  fp->ctf_str_atoms = NULL;
  fp->ctf_prov_strtab = NULL;
  memset (&fp->ctf_dtdefs, 0, sizeof (ctf_list_t));
  fp->ctf_add_processing = NULL;
  fp->ctf_ptrtab = NULL;
  fp->ctf_link_inputs = NULL;
  fp->ctf_link_outputs = NULL;
  fp->ctf_syn_ext_strtab = NULL;
  fp->ctf_link_cu_mapping = NULL;
  fp->ctf_link_type_mapping = NULL;

  fp->ctf_dvhash = NULL;
  memset (&fp->ctf_dvdefs, 0, sizeof (ctf_list_t));
  memset (fp->ctf_lookups, 0, sizeof (fp->ctf_lookups));
  fp->ctf_structs.ctn_writable = NULL;
  fp->ctf_unions.ctn_writable = NULL;
  fp->ctf_enums.ctn_writable = NULL;
  fp->ctf_names.ctn_writable = NULL;

  memcpy (&ofp, fp, sizeof (ctf_file_t));
  memcpy (fp, nfp, sizeof (ctf_file_t));
  memcpy (nfp, &ofp, sizeof (ctf_file_t));

  nfp->ctf_refcnt = 1;		/* Force nfp to be freed.  */
  ctf_file_close (nfp);

  return 0;
}

ctf_names_t *
ctf_name_table (ctf_file_t *fp, int kind)
{
  switch (kind)
    {
    case CTF_K_STRUCT:
      return &fp->ctf_structs;
    case CTF_K_UNION:
      return &fp->ctf_unions;
    case CTF_K_ENUM:
      return &fp->ctf_enums;
    default:
      return &fp->ctf_names;
    }
}

int
ctf_dtd_insert (ctf_file_t *fp, ctf_dtdef_t *dtd, int kind)
{
  const char *name;
  if (ctf_dynhash_insert (fp->ctf_dthash, (void *) dtd->dtd_type, dtd) < 0)
    return -1;

  if (dtd->dtd_data.ctt_name
      && (name = ctf_strraw (fp, dtd->dtd_data.ctt_name)) != NULL)
    {
      if (ctf_dynhash_insert (ctf_name_table (fp, kind)->ctn_writable,
			      (char *) name, (void *) dtd->dtd_type) < 0)
	{
	  ctf_dynhash_remove (fp->ctf_dthash, (void *) dtd->dtd_type);
	  return -1;
	}
    }
  ctf_list_append (&fp->ctf_dtdefs, dtd);
  return 0;
}

void
ctf_dtd_delete (ctf_file_t *fp, ctf_dtdef_t *dtd)
{
  ctf_dmdef_t *dmd, *nmd;
  int kind = LCTF_INFO_KIND (fp, dtd->dtd_data.ctt_info);
  const char *name;

  ctf_dynhash_remove (fp->ctf_dthash, (void *) dtd->dtd_type);

  switch (kind)
    {
    case CTF_K_STRUCT:
    case CTF_K_UNION:
    case CTF_K_ENUM:
      for (dmd = ctf_list_next (&dtd->dtd_u.dtu_members);
	   dmd != NULL; dmd = nmd)
	{
	  if (dmd->dmd_name != NULL)
	      free (dmd->dmd_name);
	  nmd = ctf_list_next (dmd);
	  free (dmd);
	}
      break;
    case CTF_K_FUNCTION:
      free (dtd->dtd_u.dtu_argv);
      break;
    }

  if (dtd->dtd_data.ctt_name
      && (name = ctf_strraw (fp, dtd->dtd_data.ctt_name)) != NULL)
    {
      ctf_dynhash_remove (ctf_name_table (fp, kind)->ctn_writable,
			  name);
      ctf_str_remove_ref (fp, name, &dtd->dtd_data.ctt_name);
    }

  ctf_list_delete (&fp->ctf_dtdefs, dtd);
  free (dtd);
}

ctf_dtdef_t *
ctf_dtd_lookup (const ctf_file_t *fp, ctf_id_t type)
{
  return (ctf_dtdef_t *) ctf_dynhash_lookup (fp->ctf_dthash, (void *) type);
}

ctf_dtdef_t *
ctf_dynamic_type (const ctf_file_t *fp, ctf_id_t id)
{
  ctf_id_t idx;

  if (!(fp->ctf_flags & LCTF_RDWR))
    return NULL;

  if ((fp->ctf_flags & LCTF_CHILD) && LCTF_TYPE_ISPARENT (fp, id))
    fp = fp->ctf_parent;

  idx = LCTF_TYPE_TO_INDEX(fp, id);

  if ((unsigned long) idx <= fp->ctf_typemax)
    return ctf_dtd_lookup (fp, id);
  return NULL;
}

int
ctf_dvd_insert (ctf_file_t *fp, ctf_dvdef_t *dvd)
{
  if (ctf_dynhash_insert (fp->ctf_dvhash, dvd->dvd_name, dvd) < 0)
    return -1;
  ctf_list_append (&fp->ctf_dvdefs, dvd);
  return 0;
}

void
ctf_dvd_delete (ctf_file_t *fp, ctf_dvdef_t *dvd)
{
  ctf_dynhash_remove (fp->ctf_dvhash, dvd->dvd_name);
  free (dvd->dvd_name);

  ctf_list_delete (&fp->ctf_dvdefs, dvd);
  free (dvd);
}

ctf_dvdef_t *
ctf_dvd_lookup (const ctf_file_t *fp, const char *name)
{
  return (ctf_dvdef_t *) ctf_dynhash_lookup (fp->ctf_dvhash, name);
}

/* Discard all of the dynamic type definitions and variable definitions that
   have been added to the container since the last call to ctf_update().  We
   locate such types by scanning the dtd list and deleting elements that have
   type IDs greater than ctf_dtoldid, which is set by ctf_update(), above, and
   by scanning the variable list and deleting elements that have update IDs
   equal to the current value of the last-update snapshot count (indicating that
   they were added after the most recent call to ctf_update()).  */
int
ctf_discard (ctf_file_t *fp)
{
  ctf_snapshot_id_t last_update =
    { fp->ctf_dtoldid,
      fp->ctf_snapshot_lu + 1 };

  /* Update required?  */
  if (!(fp->ctf_flags & LCTF_DIRTY))
    return 0;

  return (ctf_rollback (fp, last_update));
}

ctf_snapshot_id_t
ctf_snapshot (ctf_file_t *fp)
{
  ctf_snapshot_id_t snapid;
  snapid.dtd_id = fp->ctf_typemax;
  snapid.snapshot_id = fp->ctf_snapshots++;
  return snapid;
}

/* Like ctf_discard(), only discards everything after a particular ID.  */
int
ctf_rollback (ctf_file_t *fp, ctf_snapshot_id_t id)
{
  ctf_dtdef_t *dtd, *ntd;
  ctf_dvdef_t *dvd, *nvd;

  if (!(fp->ctf_flags & LCTF_RDWR))
    return (ctf_set_errno (fp, ECTF_RDONLY));

  if (fp->ctf_snapshot_lu >= id.snapshot_id)
    return (ctf_set_errno (fp, ECTF_OVERROLLBACK));

  for (dtd = ctf_list_next (&fp->ctf_dtdefs); dtd != NULL; dtd = ntd)
    {
      int kind;
      const char *name;

      ntd = ctf_list_next (dtd);

      if (LCTF_TYPE_TO_INDEX (fp, dtd->dtd_type) <= id.dtd_id)
	continue;

      kind = LCTF_INFO_KIND (fp, dtd->dtd_data.ctt_info);

      if (dtd->dtd_data.ctt_name
	  && (name = ctf_strraw (fp, dtd->dtd_data.ctt_name)) != NULL)
	{
	  ctf_dynhash_remove (ctf_name_table (fp, kind)->ctn_writable,
			      name);
	  ctf_str_remove_ref (fp, name, &dtd->dtd_data.ctt_name);
	}

      ctf_dynhash_remove (fp->ctf_dthash, (void *) dtd->dtd_type);
      ctf_dtd_delete (fp, dtd);
    }

  for (dvd = ctf_list_next (&fp->ctf_dvdefs); dvd != NULL; dvd = nvd)
    {
      nvd = ctf_list_next (dvd);

      if (dvd->dvd_snapshots <= id.snapshot_id)
	continue;

      ctf_dvd_delete (fp, dvd);
    }

  fp->ctf_typemax = id.dtd_id;
  fp->ctf_snapshots = id.snapshot_id;

  if (fp->ctf_snapshots == fp->ctf_snapshot_lu)
    fp->ctf_flags &= ~LCTF_DIRTY;

  return 0;
}

static ctf_id_t
ctf_add_generic (ctf_file_t *fp, uint32_t flag, const char *name, int kind,
		 ctf_dtdef_t **rp)
{
  ctf_dtdef_t *dtd;
  ctf_id_t type;

  if (flag != CTF_ADD_NONROOT && flag != CTF_ADD_ROOT)
    return (ctf_set_errno (fp, EINVAL));

  if (!(fp->ctf_flags & LCTF_RDWR))
    return (ctf_set_errno (fp, ECTF_RDONLY));

  if (LCTF_INDEX_TO_TYPE (fp, fp->ctf_typemax, 1) >= CTF_MAX_TYPE)
    return (ctf_set_errno (fp, ECTF_FULL));

  if (LCTF_INDEX_TO_TYPE (fp, fp->ctf_typemax, 1) == (CTF_MAX_PTYPE - 1))
    return (ctf_set_errno (fp, ECTF_FULL));

  /* Make sure ptrtab always grows to be big enough for all types.  */
  if (ctf_grow_ptrtab (fp) < 0)
      return CTF_ERR;		/* errno is set for us. */

  if ((dtd = malloc (sizeof (ctf_dtdef_t))) == NULL)
    return (ctf_set_errno (fp, EAGAIN));

  type = ++fp->ctf_typemax;
  type = LCTF_INDEX_TO_TYPE (fp, type, (fp->ctf_flags & LCTF_CHILD));

  memset (dtd, 0, sizeof (ctf_dtdef_t));
  dtd->dtd_data.ctt_name = ctf_str_add_ref (fp, name, &dtd->dtd_data.ctt_name);
  dtd->dtd_type = type;

  if (dtd->dtd_data.ctt_name == 0 && name != NULL && name[0] != '\0')
    {
      free (dtd);
      return (ctf_set_errno (fp, EAGAIN));
    }

  if (ctf_dtd_insert (fp, dtd, kind) < 0)
    {
      free (dtd);
      return CTF_ERR;			/* errno is set for us.  */
    }
  fp->ctf_flags |= LCTF_DIRTY;

  *rp = dtd;
  return type;
}

/* When encoding integer sizes, we want to convert a byte count in the range
   1-8 to the closest power of 2 (e.g. 3->4, 5->8, etc).  The clp2() function
   is a clever implementation from "Hacker's Delight" by Henry Warren, Jr.  */
static size_t
clp2 (size_t x)
{
  x--;

  x |= (x >> 1);
  x |= (x >> 2);
  x |= (x >> 4);
  x |= (x >> 8);
  x |= (x >> 16);

  return (x + 1);
}

static ctf_id_t
ctf_add_encoded (ctf_file_t *fp, uint32_t flag,
		 const char *name, const ctf_encoding_t *ep, uint32_t kind)
{
  ctf_dtdef_t *dtd;
  ctf_id_t type;

  if (ep == NULL)
    return (ctf_set_errno (fp, EINVAL));

  if ((type = ctf_add_generic (fp, flag, name, kind, &dtd)) == CTF_ERR)
    return CTF_ERR;		/* errno is set for us.  */

  dtd->dtd_data.ctt_info = CTF_TYPE_INFO (kind, flag, 0);
  dtd->dtd_data.ctt_size = clp2 (P2ROUNDUP (ep->cte_bits, CHAR_BIT)
				 / CHAR_BIT);
  dtd->dtd_u.dtu_enc = *ep;

  return type;
}

static ctf_id_t
ctf_add_reftype (ctf_file_t *fp, uint32_t flag, ctf_id_t ref, uint32_t kind)
{
  ctf_dtdef_t *dtd;
  ctf_id_t type;
  ctf_file_t *tmp = fp;
  int child = fp->ctf_flags & LCTF_CHILD;

  if (ref == CTF_ERR || ref > CTF_MAX_TYPE)
    return (ctf_set_errno (fp, EINVAL));

  if (ctf_lookup_by_id (&tmp, ref) == NULL)
    return CTF_ERR;		/* errno is set for us.  */

  if ((type = ctf_add_generic (fp, flag, NULL, kind, &dtd)) == CTF_ERR)
    return CTF_ERR;		/* errno is set for us.  */

  dtd->dtd_data.ctt_info = CTF_TYPE_INFO (kind, flag, 0);
  dtd->dtd_data.ctt_type = (uint32_t) ref;

  if (kind != CTF_K_POINTER)
    return type;

  /* If we are adding a pointer, update the ptrtab, both the directly pointed-to
     type and (if an anonymous typedef node is being pointed at) the type that
     points at too.  Note that ctf_typemax is at this point one higher than we
     want to check against, because it's just been incremented for the addition
     of this type.  */

  uint32_t type_idx = LCTF_TYPE_TO_INDEX (fp, type);
  uint32_t ref_idx = LCTF_TYPE_TO_INDEX (fp, ref);

  if (LCTF_TYPE_ISCHILD (fp, ref) == child
      && ref_idx < fp->ctf_typemax)
    {
      fp->ctf_ptrtab[ref_idx] = type_idx;

      ctf_id_t refref_idx = LCTF_TYPE_TO_INDEX (fp, dtd->dtd_data.ctt_type);

      if (tmp == fp
	  && (LCTF_INFO_KIND (fp, dtd->dtd_data.ctt_info) == CTF_K_TYPEDEF)
	  && strcmp (ctf_strptr (fp, dtd->dtd_data.ctt_name), "") == 0
	  && refref_idx < fp->ctf_typemax)
	fp->ctf_ptrtab[refref_idx] = type_idx;
    }

  return type;
}

ctf_id_t
ctf_add_slice (ctf_file_t *fp, uint32_t flag, ctf_id_t ref,
	       const ctf_encoding_t *ep)
{
  ctf_dtdef_t *dtd;
  ctf_id_t type;
  int kind;
  const ctf_type_t *tp;
  ctf_file_t *tmp = fp;

  if (ep == NULL)
    return (ctf_set_errno (fp, EINVAL));

  if ((ep->cte_bits > 255) || (ep->cte_offset > 255))
    return (ctf_set_errno (fp, ECTF_SLICEOVERFLOW));

  if (ref == CTF_ERR || ref > CTF_MAX_TYPE)
    return (ctf_set_errno (fp, EINVAL));

  if ((tp = ctf_lookup_by_id (&tmp, ref)) == NULL)
    return CTF_ERR;		/* errno is set for us.  */

  kind = ctf_type_kind_unsliced (tmp, ref);
  if ((kind != CTF_K_INTEGER) && (kind != CTF_K_FLOAT) &&
      (kind != CTF_K_ENUM))
    return (ctf_set_errno (fp, ECTF_NOTINTFP));

  if ((type = ctf_add_generic (fp, flag, NULL, CTF_K_SLICE, &dtd)) == CTF_ERR)
    return CTF_ERR;		/* errno is set for us.  */

  dtd->dtd_data.ctt_info = CTF_TYPE_INFO (CTF_K_SLICE, flag, 0);
  dtd->dtd_data.ctt_size = clp2 (P2ROUNDUP (ep->cte_bits, CHAR_BIT)
				 / CHAR_BIT);
  dtd->dtd_u.dtu_slice.cts_type = ref;
  dtd->dtd_u.dtu_slice.cts_bits = ep->cte_bits;
  dtd->dtd_u.dtu_slice.cts_offset = ep->cte_offset;

  return type;
}

ctf_id_t
ctf_add_integer (ctf_file_t *fp, uint32_t flag,
		 const char *name, const ctf_encoding_t *ep)
{
  return (ctf_add_encoded (fp, flag, name, ep, CTF_K_INTEGER));
}

ctf_id_t
ctf_add_float (ctf_file_t *fp, uint32_t flag,
	       const char *name, const ctf_encoding_t *ep)
{
  return (ctf_add_encoded (fp, flag, name, ep, CTF_K_FLOAT));
}

ctf_id_t
ctf_add_pointer (ctf_file_t *fp, uint32_t flag, ctf_id_t ref)
{
  return (ctf_add_reftype (fp, flag, ref, CTF_K_POINTER));
}

ctf_id_t
ctf_add_array (ctf_file_t *fp, uint32_t flag, const ctf_arinfo_t *arp)
{
  ctf_dtdef_t *dtd;
  ctf_id_t type;
  ctf_file_t *tmp = fp;

  if (arp == NULL)
    return (ctf_set_errno (fp, EINVAL));

  if (ctf_lookup_by_id (&tmp, arp->ctr_contents) == NULL)
    return CTF_ERR;		/* errno is set for us.  */

  tmp = fp;
  if (ctf_lookup_by_id (&tmp, arp->ctr_index) == NULL)
    return CTF_ERR;		/* errno is set for us.  */

  if ((type = ctf_add_generic (fp, flag, NULL, CTF_K_ARRAY, &dtd)) == CTF_ERR)
    return CTF_ERR;		/* errno is set for us.  */

  dtd->dtd_data.ctt_info = CTF_TYPE_INFO (CTF_K_ARRAY, flag, 0);
  dtd->dtd_data.ctt_size = 0;
  dtd->dtd_u.dtu_arr = *arp;

  return type;
}

int
ctf_set_array (ctf_file_t *fp, ctf_id_t type, const ctf_arinfo_t *arp)
{
  ctf_dtdef_t *dtd = ctf_dtd_lookup (fp, type);

  if (!(fp->ctf_flags & LCTF_RDWR))
    return (ctf_set_errno (fp, ECTF_RDONLY));

  if (dtd == NULL
      || LCTF_INFO_KIND (fp, dtd->dtd_data.ctt_info) != CTF_K_ARRAY)
    return (ctf_set_errno (fp, ECTF_BADID));

  fp->ctf_flags |= LCTF_DIRTY;
  dtd->dtd_u.dtu_arr = *arp;

  return 0;
}

ctf_id_t
ctf_add_function (ctf_file_t *fp, uint32_t flag,
		  const ctf_funcinfo_t *ctc, const ctf_id_t *argv)
{
  ctf_dtdef_t *dtd;
  ctf_id_t type;
  uint32_t vlen;
  ctf_id_t *vdat = NULL;
  ctf_file_t *tmp = fp;
  size_t i;

  if (ctc == NULL || (ctc->ctc_flags & ~CTF_FUNC_VARARG) != 0
      || (ctc->ctc_argc != 0 && argv == NULL))
    return (ctf_set_errno (fp, EINVAL));

  vlen = ctc->ctc_argc;
  if (ctc->ctc_flags & CTF_FUNC_VARARG)
    vlen++;	       /* Add trailing zero to indicate varargs (see below).  */

  if (ctf_lookup_by_id (&tmp, ctc->ctc_return) == NULL)
    return CTF_ERR;		/* errno is set for us.  */

  for (i = 0; i < ctc->ctc_argc; i++)
    {
      tmp = fp;
      if (ctf_lookup_by_id (&tmp, argv[i]) == NULL)
	return CTF_ERR;		/* errno is set for us.  */
    }

  if (vlen > CTF_MAX_VLEN)
    return (ctf_set_errno (fp, EOVERFLOW));

  if (vlen != 0 && (vdat = malloc (sizeof (ctf_id_t) * vlen)) == NULL)
    return (ctf_set_errno (fp, EAGAIN));

  if ((type = ctf_add_generic (fp, flag, NULL, CTF_K_FUNCTION,
			       &dtd)) == CTF_ERR)
    {
      free (vdat);
      return CTF_ERR;		   /* errno is set for us.  */
    }

  dtd->dtd_data.ctt_info = CTF_TYPE_INFO (CTF_K_FUNCTION, flag, vlen);
  dtd->dtd_data.ctt_type = (uint32_t) ctc->ctc_return;

  memcpy (vdat, argv, sizeof (ctf_id_t) * ctc->ctc_argc);
  if (ctc->ctc_flags & CTF_FUNC_VARARG)
    vdat[vlen - 1] = 0;		   /* Add trailing zero to indicate varargs.  */
  dtd->dtd_u.dtu_argv = vdat;

  return type;
}

ctf_id_t
ctf_add_struct_sized (ctf_file_t *fp, uint32_t flag, const char *name,
		      size_t size)
{
  ctf_dtdef_t *dtd;
  ctf_id_t type = 0;

  /* Promote forwards to structs.  */

  if (name != NULL)
    type = ctf_lookup_by_rawname (fp, CTF_K_STRUCT, name);

  if (type != 0 && ctf_type_kind (fp, type) == CTF_K_FORWARD)
    dtd = ctf_dtd_lookup (fp, type);
  else if ((type = ctf_add_generic (fp, flag, name, CTF_K_STRUCT,
				    &dtd)) == CTF_ERR)
    return CTF_ERR;		/* errno is set for us.  */

  dtd->dtd_data.ctt_info = CTF_TYPE_INFO (CTF_K_STRUCT, flag, 0);

  if (size > CTF_MAX_SIZE)
    {
      dtd->dtd_data.ctt_size = CTF_LSIZE_SENT;
      dtd->dtd_data.ctt_lsizehi = CTF_SIZE_TO_LSIZE_HI (size);
      dtd->dtd_data.ctt_lsizelo = CTF_SIZE_TO_LSIZE_LO (size);
    }
  else
    dtd->dtd_data.ctt_size = (uint32_t) size;

  return type;
}

ctf_id_t
ctf_add_struct (ctf_file_t *fp, uint32_t flag, const char *name)
{
  return (ctf_add_struct_sized (fp, flag, name, 0));
}

ctf_id_t
ctf_add_union_sized (ctf_file_t *fp, uint32_t flag, const char *name,
		     size_t size)
{
  ctf_dtdef_t *dtd;
  ctf_id_t type = 0;

  /* Promote forwards to unions.  */
  if (name != NULL)
    type = ctf_lookup_by_rawname (fp, CTF_K_UNION, name);

  if (type != 0 && ctf_type_kind (fp, type) == CTF_K_FORWARD)
    dtd = ctf_dtd_lookup (fp, type);
  else if ((type = ctf_add_generic (fp, flag, name, CTF_K_UNION,
				    &dtd)) == CTF_ERR)
    return CTF_ERR;		/* errno is set for us */

  dtd->dtd_data.ctt_info = CTF_TYPE_INFO (CTF_K_UNION, flag, 0);

  if (size > CTF_MAX_SIZE)
    {
      dtd->dtd_data.ctt_size = CTF_LSIZE_SENT;
      dtd->dtd_data.ctt_lsizehi = CTF_SIZE_TO_LSIZE_HI (size);
      dtd->dtd_data.ctt_lsizelo = CTF_SIZE_TO_LSIZE_LO (size);
    }
  else
    dtd->dtd_data.ctt_size = (uint32_t) size;

  return type;
}

ctf_id_t
ctf_add_union (ctf_file_t *fp, uint32_t flag, const char *name)
{
  return (ctf_add_union_sized (fp, flag, name, 0));
}

ctf_id_t
ctf_add_enum (ctf_file_t *fp, uint32_t flag, const char *name)
{
  ctf_dtdef_t *dtd;
  ctf_id_t type = 0;

  /* Promote forwards to enums.  */
  if (name != NULL)
    type = ctf_lookup_by_rawname (fp, CTF_K_ENUM, name);

  if (type != 0 && ctf_type_kind (fp, type) == CTF_K_FORWARD)
    dtd = ctf_dtd_lookup (fp, type);
  else if ((type = ctf_add_generic (fp, flag, name, CTF_K_ENUM,
				    &dtd)) == CTF_ERR)
    return CTF_ERR;		/* errno is set for us.  */

  dtd->dtd_data.ctt_info = CTF_TYPE_INFO (CTF_K_ENUM, flag, 0);
  dtd->dtd_data.ctt_size = fp->ctf_dmodel->ctd_int;

  return type;
}

ctf_id_t
ctf_add_enum_encoded (ctf_file_t *fp, uint32_t flag, const char *name,
		      const ctf_encoding_t *ep)
{
  ctf_id_t type = 0;

  /* First, create the enum if need be, using most of the same machinery as
     ctf_add_enum(), to ensure that we do not allow things past that are not
     enums or forwards to them.  (This includes other slices: you cannot slice a
     slice, which would be a useless thing to do anyway.)  */

  if (name != NULL)
    type = ctf_lookup_by_rawname (fp, CTF_K_ENUM, name);

  if (type != 0)
    {
      if ((ctf_type_kind (fp, type) != CTF_K_FORWARD) &&
	  (ctf_type_kind_unsliced (fp, type) != CTF_K_ENUM))
	return (ctf_set_errno (fp, ECTF_NOTINTFP));
    }
  else if ((type = ctf_add_enum (fp, flag, name)) == CTF_ERR)
    return CTF_ERR;		/* errno is set for us.  */

  /* Now attach a suitable slice to it.  */

  return ctf_add_slice (fp, flag, type, ep);
}

ctf_id_t
ctf_add_forward (ctf_file_t *fp, uint32_t flag, const char *name,
		 uint32_t kind)
{
  ctf_dtdef_t *dtd;
  ctf_id_t type = 0;

  if (kind != CTF_K_STRUCT && kind != CTF_K_UNION && kind != CTF_K_ENUM)
    return (ctf_set_errno (fp, ECTF_NOTSUE));

  /* If the type is already defined or exists as a forward tag, just
     return the ctf_id_t of the existing definition.  */

  if (name != NULL)
    type = ctf_lookup_by_rawname (fp, kind, name);

  if ((type = ctf_add_generic (fp, flag, name, CTF_K_FORWARD,&dtd)) == CTF_ERR)
    return CTF_ERR;		/* errno is set for us.  */

  dtd->dtd_data.ctt_info = CTF_TYPE_INFO (CTF_K_FORWARD, flag, 0);
  dtd->dtd_data.ctt_type = kind;

  return type;
}

ctf_id_t
ctf_add_typedef (ctf_file_t *fp, uint32_t flag, const char *name,
		 ctf_id_t ref)
{
  ctf_dtdef_t *dtd;
  ctf_id_t type;
  ctf_file_t *tmp = fp;

  if (ref == CTF_ERR || ref > CTF_MAX_TYPE)
    return (ctf_set_errno (fp, EINVAL));

  if (ctf_lookup_by_id (&tmp, ref) == NULL)
    return CTF_ERR;		/* errno is set for us.  */

  if ((type = ctf_add_generic (fp, flag, name, CTF_K_TYPEDEF,
			       &dtd)) == CTF_ERR)
    return CTF_ERR;		/* errno is set for us.  */

  dtd->dtd_data.ctt_info = CTF_TYPE_INFO (CTF_K_TYPEDEF, flag, 0);
  dtd->dtd_data.ctt_type = (uint32_t) ref;

  return type;
}

ctf_id_t
ctf_add_volatile (ctf_file_t *fp, uint32_t flag, ctf_id_t ref)
{
  return (ctf_add_reftype (fp, flag, ref, CTF_K_VOLATILE));
}

ctf_id_t
ctf_add_const (ctf_file_t *fp, uint32_t flag, ctf_id_t ref)
{
  return (ctf_add_reftype (fp, flag, ref, CTF_K_CONST));
}

ctf_id_t
ctf_add_restrict (ctf_file_t *fp, uint32_t flag, ctf_id_t ref)
{
  return (ctf_add_reftype (fp, flag, ref, CTF_K_RESTRICT));
}

int
ctf_add_enumerator (ctf_file_t *fp, ctf_id_t enid, const char *name,
		    int value)
{
  ctf_dtdef_t *dtd = ctf_dtd_lookup (fp, enid);
  ctf_dmdef_t *dmd;

  uint32_t kind, vlen, root;
  char *s;

  if (name == NULL)
    return (ctf_set_errno (fp, EINVAL));

  if (!(fp->ctf_flags & LCTF_RDWR))
    return (ctf_set_errno (fp, ECTF_RDONLY));

  if (dtd == NULL)
    return (ctf_set_errno (fp, ECTF_BADID));

  kind = LCTF_INFO_KIND (fp, dtd->dtd_data.ctt_info);
  root = LCTF_INFO_ISROOT (fp, dtd->dtd_data.ctt_info);
  vlen = LCTF_INFO_VLEN (fp, dtd->dtd_data.ctt_info);

  if (kind != CTF_K_ENUM)
    return (ctf_set_errno (fp, ECTF_NOTENUM));

  if (vlen == CTF_MAX_VLEN)
    return (ctf_set_errno (fp, ECTF_DTFULL));

  for (dmd = ctf_list_next (&dtd->dtd_u.dtu_members);
       dmd != NULL; dmd = ctf_list_next (dmd))
    {
      if (strcmp (dmd->dmd_name, name) == 0)
	return (ctf_set_errno (fp, ECTF_DUPLICATE));
    }

  if ((dmd = malloc (sizeof (ctf_dmdef_t))) == NULL)
    return (ctf_set_errno (fp, EAGAIN));

  if ((s = strdup (name)) == NULL)
    {
      free (dmd);
      return (ctf_set_errno (fp, EAGAIN));
    }

  dmd->dmd_name = s;
  dmd->dmd_type = CTF_ERR;
  dmd->dmd_offset = 0;
  dmd->dmd_value = value;

  dtd->dtd_data.ctt_info = CTF_TYPE_INFO (kind, root, vlen + 1);
  ctf_list_append (&dtd->dtd_u.dtu_members, dmd);

  fp->ctf_flags |= LCTF_DIRTY;

  return 0;
}

int
ctf_add_member_offset (ctf_file_t *fp, ctf_id_t souid, const char *name,
		       ctf_id_t type, unsigned long bit_offset)
{
  ctf_dtdef_t *dtd = ctf_dtd_lookup (fp, souid);
  ctf_dmdef_t *dmd;

  ssize_t msize, malign, ssize;
  uint32_t kind, vlen, root;
  char *s = NULL;

  if (!(fp->ctf_flags & LCTF_RDWR))
    return (ctf_set_errno (fp, ECTF_RDONLY));

  if (dtd == NULL)
    return (ctf_set_errno (fp, ECTF_BADID));

  kind = LCTF_INFO_KIND (fp, dtd->dtd_data.ctt_info);
  root = LCTF_INFO_ISROOT (fp, dtd->dtd_data.ctt_info);
  vlen = LCTF_INFO_VLEN (fp, dtd->dtd_data.ctt_info);

  if (kind != CTF_K_STRUCT && kind != CTF_K_UNION)
    return (ctf_set_errno (fp, ECTF_NOTSOU));

  if (vlen == CTF_MAX_VLEN)
    return (ctf_set_errno (fp, ECTF_DTFULL));

  if (name != NULL)
    {
      for (dmd = ctf_list_next (&dtd->dtd_u.dtu_members);
	   dmd != NULL; dmd = ctf_list_next (dmd))
	{
	  if (dmd->dmd_name != NULL && strcmp (dmd->dmd_name, name) == 0)
	    return (ctf_set_errno (fp, ECTF_DUPLICATE));
	}
    }

  if ((msize = ctf_type_size (fp, type)) < 0 ||
      (malign = ctf_type_align (fp, type)) < 0)
    return -1;			/* errno is set for us.  */

  if ((dmd = malloc (sizeof (ctf_dmdef_t))) == NULL)
    return (ctf_set_errno (fp, EAGAIN));

  if (name != NULL && (s = strdup (name)) == NULL)
    {
      free (dmd);
      return (ctf_set_errno (fp, EAGAIN));
    }

  dmd->dmd_name = s;
  dmd->dmd_type = type;
  dmd->dmd_value = -1;

  if (kind == CTF_K_STRUCT && vlen != 0)
    {
      if (bit_offset == (unsigned long) - 1)
	{
	  /* Natural alignment.  */

	  ctf_dmdef_t *lmd = ctf_list_prev (&dtd->dtd_u.dtu_members);
	  ctf_id_t ltype = ctf_type_resolve (fp, lmd->dmd_type);
	  size_t off = lmd->dmd_offset;

	  ctf_encoding_t linfo;
	  ssize_t lsize;

	  if (ctf_type_encoding (fp, ltype, &linfo) == 0)
	    off += linfo.cte_bits;
	  else if ((lsize = ctf_type_size (fp, ltype)) > 0)
	    off += lsize * CHAR_BIT;

	  /* Round up the offset of the end of the last member to
	     the next byte boundary, convert 'off' to bytes, and
	     then round it up again to the next multiple of the
	     alignment required by the new member.  Finally,
	     convert back to bits and store the result in
	     dmd_offset.  Technically we could do more efficient
	     packing if the new member is a bit-field, but we're
	     the "compiler" and ANSI says we can do as we choose.  */

	  off = roundup (off, CHAR_BIT) / CHAR_BIT;
	  off = roundup (off, MAX (malign, 1));
	  dmd->dmd_offset = off * CHAR_BIT;
	  ssize = off + msize;
	}
      else
	{
	  /* Specified offset in bits.  */

	  dmd->dmd_offset = bit_offset;
	  ssize = ctf_get_ctt_size (fp, &dtd->dtd_data, NULL, NULL);
	  ssize = MAX (ssize, ((signed) bit_offset / CHAR_BIT) + msize);
	}
    }
  else
    {
      dmd->dmd_offset = 0;
      ssize = ctf_get_ctt_size (fp, &dtd->dtd_data, NULL, NULL);
      ssize = MAX (ssize, msize);
    }

  if ((size_t) ssize > CTF_MAX_SIZE)
    {
      dtd->dtd_data.ctt_size = CTF_LSIZE_SENT;
      dtd->dtd_data.ctt_lsizehi = CTF_SIZE_TO_LSIZE_HI (ssize);
      dtd->dtd_data.ctt_lsizelo = CTF_SIZE_TO_LSIZE_LO (ssize);
    }
  else
    dtd->dtd_data.ctt_size = (uint32_t) ssize;

  dtd->dtd_data.ctt_info = CTF_TYPE_INFO (kind, root, vlen + 1);
  ctf_list_append (&dtd->dtd_u.dtu_members, dmd);

  fp->ctf_flags |= LCTF_DIRTY;
  return 0;
}

int
ctf_add_member_encoded (ctf_file_t *fp, ctf_id_t souid, const char *name,
			ctf_id_t type, unsigned long bit_offset,
			const ctf_encoding_t encoding)
{
  ctf_dtdef_t *dtd = ctf_dtd_lookup (fp, type);
  int kind = LCTF_INFO_KIND (fp, dtd->dtd_data.ctt_info);
  int otype = type;

  if ((kind != CTF_K_INTEGER) && (kind != CTF_K_FLOAT) && (kind != CTF_K_ENUM))
    return (ctf_set_errno (fp, ECTF_NOTINTFP));

  if ((type = ctf_add_slice (fp, CTF_ADD_NONROOT, otype, &encoding)) == CTF_ERR)
    return -1;			/* errno is set for us.  */

  return ctf_add_member_offset (fp, souid, name, type, bit_offset);
}

int
ctf_add_member (ctf_file_t *fp, ctf_id_t souid, const char *name,
		ctf_id_t type)
{
  return ctf_add_member_offset (fp, souid, name, type, (unsigned long) - 1);
}

int
ctf_add_variable (ctf_file_t *fp, const char *name, ctf_id_t ref)
{
  ctf_dvdef_t *dvd;
  ctf_file_t *tmp = fp;

  if (!(fp->ctf_flags & LCTF_RDWR))
    return (ctf_set_errno (fp, ECTF_RDONLY));

  if (ctf_dvd_lookup (fp, name) != NULL)
    return (ctf_set_errno (fp, ECTF_DUPLICATE));

  if (ctf_lookup_by_id (&tmp, ref) == NULL)
    return -1;			/* errno is set for us.  */

  /* Make sure this type is representable.  */
  if ((ctf_type_resolve (fp, ref) == CTF_ERR)
      && (ctf_errno (fp) == ECTF_NONREPRESENTABLE))
    return -1;

  if ((dvd = malloc (sizeof (ctf_dvdef_t))) == NULL)
    return (ctf_set_errno (fp, EAGAIN));

  if (name != NULL && (dvd->dvd_name = strdup (name)) == NULL)
    {
      free (dvd);
      return (ctf_set_errno (fp, EAGAIN));
    }
  dvd->dvd_type = ref;
  dvd->dvd_snapshots = fp->ctf_snapshots;

  if (ctf_dvd_insert (fp, dvd) < 0)
    {
      free (dvd->dvd_name);
      free (dvd);
      return -1;			/* errno is set for us.  */
    }

  fp->ctf_flags |= LCTF_DIRTY;
  return 0;
}

static int
enumcmp (const char *name, int value, void *arg)
{
  ctf_bundle_t *ctb = arg;
  int bvalue;

  if (ctf_enum_value (ctb->ctb_file, ctb->ctb_type, name, &bvalue) < 0)
    {
      ctf_dprintf ("Conflict due to member %s iteration error: %s.\n", name,
		   ctf_errmsg (ctf_errno (ctb->ctb_file)));
      return 1;
    }
  if (value != bvalue)
    {
      ctf_dprintf ("Conflict due to value change: %i versus %i\n",
		   value, bvalue);
      return 1;
    }
  return 0;
}

static int
enumadd (const char *name, int value, void *arg)
{
  ctf_bundle_t *ctb = arg;

  return (ctf_add_enumerator (ctb->ctb_file, ctb->ctb_type,
			      name, value) < 0);
}

static int
membcmp (const char *name, ctf_id_t type _libctf_unused_, unsigned long offset,
	 void *arg)
{
  ctf_bundle_t *ctb = arg;
  ctf_membinfo_t ctm;

  if (ctf_member_info (ctb->ctb_file, ctb->ctb_type, name, &ctm) < 0)
    {
      ctf_dprintf ("Conflict due to member %s iteration error: %s.\n", name,
		   ctf_errmsg (ctf_errno (ctb->ctb_file)));
      return 1;
    }
  if (ctm.ctm_offset != offset)
    {
      ctf_dprintf ("Conflict due to member %s offset change: "
		   "%lx versus %lx\n", name, ctm.ctm_offset, offset);
      return 1;
    }
  return 0;
}

static int
membadd (const char *name, ctf_id_t type, unsigned long offset, void *arg)
{
  ctf_bundle_t *ctb = arg;
  ctf_dmdef_t *dmd;
  char *s = NULL;

  if ((dmd = malloc (sizeof (ctf_dmdef_t))) == NULL)
    return (ctf_set_errno (ctb->ctb_file, EAGAIN));

  if (name != NULL && (s = strdup (name)) == NULL)
    {
      free (dmd);
      return (ctf_set_errno (ctb->ctb_file, EAGAIN));
    }

  /* For now, dmd_type is copied as the src_fp's type; it is reset to an
    equivalent dst_fp type by a final loop in ctf_add_type(), below.  */
  dmd->dmd_name = s;
  dmd->dmd_type = type;
  dmd->dmd_offset = offset;
  dmd->dmd_value = -1;

  ctf_list_append (&ctb->ctb_dtd->dtd_u.dtu_members, dmd);

  ctb->ctb_file->ctf_flags |= LCTF_DIRTY;
  return 0;
}

/* The ctf_add_type routine is used to copy a type from a source CTF container
   to a dynamic destination container.  This routine operates recursively by
   following the source type's links and embedded member types.  If the
   destination container already contains a named type which has the same
   attributes, then we succeed and return this type but no changes occur.  */
static ctf_id_t
ctf_add_type_internal (ctf_file_t *dst_fp, ctf_file_t *src_fp, ctf_id_t src_type,
		       ctf_file_t *proc_tracking_fp)
{
  ctf_id_t dst_type = CTF_ERR;
  uint32_t dst_kind = CTF_K_UNKNOWN;
  ctf_file_t *tmp_fp = dst_fp;
  ctf_id_t tmp;

  const char *name;
  uint32_t kind, forward_kind, flag, vlen;

  const ctf_type_t *src_tp, *dst_tp;
  ctf_bundle_t src, dst;
  ctf_encoding_t src_en, dst_en;
  ctf_arinfo_t src_ar, dst_ar;

  ctf_funcinfo_t ctc;

  ctf_id_t orig_src_type = src_type;

  if (!(dst_fp->ctf_flags & LCTF_RDWR))
    return (ctf_set_errno (dst_fp, ECTF_RDONLY));

  if ((src_tp = ctf_lookup_by_id (&src_fp, src_type)) == NULL)
    return (ctf_set_errno (dst_fp, ctf_errno (src_fp)));

  if ((ctf_type_resolve (src_fp, src_type) == CTF_ERR)
      && (ctf_errno (src_fp) == ECTF_NONREPRESENTABLE))
    return (ctf_set_errno (dst_fp, ECTF_NONREPRESENTABLE));

  name = ctf_strptr (src_fp, src_tp->ctt_name);
  kind = LCTF_INFO_KIND (src_fp, src_tp->ctt_info);
  flag = LCTF_INFO_ISROOT (src_fp, src_tp->ctt_info);
  vlen = LCTF_INFO_VLEN (src_fp, src_tp->ctt_info);

  /* If this is a type we are currently in the middle of adding, hand it
     straight back.  (This lets us handle self-referential structures without
     considering forwards and empty structures the same as their completed
     forms.)  */

  tmp = ctf_type_mapping (src_fp, src_type, &tmp_fp);

  if (tmp != 0)
    {
      if (ctf_dynhash_lookup (proc_tracking_fp->ctf_add_processing,
			      (void *) (uintptr_t) src_type))
	return tmp;

      /* If this type has already been added from this container, and is the same
	 kind and (if a struct or union) has the same number of members, hand it
	 straight back.  */

      if ((ctf_type_kind_unsliced (tmp_fp, tmp) == (int) kind)
	  && (kind == CTF_K_STRUCT || kind == CTF_K_UNION
	      || kind == CTF_K_ENUM))
	{
	  if ((dst_tp = ctf_lookup_by_id (&tmp_fp, dst_type)) != NULL)
	    if (vlen == LCTF_INFO_VLEN (tmp_fp, dst_tp->ctt_info))
	      return tmp;
	}
    }

  forward_kind = kind;
  if (kind == CTF_K_FORWARD)
    forward_kind = src_tp->ctt_type;

  /* If the source type has a name and is a root type (visible at the
     top-level scope), lookup the name in the destination container and
     verify that it is of the same kind before we do anything else.  */

  if ((flag & CTF_ADD_ROOT) && name[0] != '\0'
      && (tmp = ctf_lookup_by_rawname (dst_fp, forward_kind, name)) != 0)
    {
      dst_type = tmp;
      dst_kind = ctf_type_kind_unsliced (dst_fp, dst_type);
    }

  /* If an identically named dst_type exists, fail with ECTF_CONFLICT
     unless dst_type is a forward declaration and src_type is a struct,
     union, or enum (i.e. the definition of the previous forward decl).

     We also allow addition in the opposite order (addition of a forward when a
     struct, union, or enum already exists), which is a NOP and returns the
     already-present struct, union, or enum.  */

  if (dst_type != CTF_ERR && dst_kind != kind)
    {
      if (kind == CTF_K_FORWARD
	  && (dst_kind == CTF_K_ENUM || dst_kind == CTF_K_STRUCT
	      || dst_kind == CTF_K_UNION))
	{
	  ctf_add_type_mapping (src_fp, src_type, dst_fp, dst_type);
	  return dst_type;
	}

      if (dst_kind != CTF_K_FORWARD
	  || (kind != CTF_K_ENUM && kind != CTF_K_STRUCT
	      && kind != CTF_K_UNION))
	{
	  ctf_dprintf ("Conflict for type %s: kinds differ, new: %i; "
		       "old (ID %lx): %i\n", name, kind, dst_type, dst_kind);
	  return (ctf_set_errno (dst_fp, ECTF_CONFLICT));
	}
    }

  /* We take special action for an integer, float, or slice since it is
     described not only by its name but also its encoding.  For integers,
     bit-fields exploit this degeneracy.  */

  if (kind == CTF_K_INTEGER || kind == CTF_K_FLOAT || kind == CTF_K_SLICE)
    {
      if (ctf_type_encoding (src_fp, src_type, &src_en) != 0)
	return (ctf_set_errno (dst_fp, ctf_errno (src_fp)));

      if (dst_type != CTF_ERR)
	{
	  ctf_file_t *fp = dst_fp;

	  if ((dst_tp = ctf_lookup_by_id (&fp, dst_type)) == NULL)
	    return CTF_ERR;

	  if (ctf_type_encoding (dst_fp, dst_type, &dst_en) != 0)
	    return CTF_ERR;			/* errno set for us.  */

	  if (LCTF_INFO_ISROOT (fp, dst_tp->ctt_info) & CTF_ADD_ROOT)
	    {
	      /* The type that we found in the hash is also root-visible.  If
		 the two types match then use the existing one; otherwise,
		 declare a conflict.  Note: slices are not certain to match
		 even if there is no conflict: we must check the contained type
		 too.  */

	      if (memcmp (&src_en, &dst_en, sizeof (ctf_encoding_t)) == 0)
		{
		  if (kind != CTF_K_SLICE)
		    {
		      ctf_add_type_mapping (src_fp, src_type, dst_fp, dst_type);
		      return dst_type;
		    }
		}
	      else
		  {
		    return (ctf_set_errno (dst_fp, ECTF_CONFLICT));
		  }
	    }
	  else
	    {
	      /* We found a non-root-visible type in the hash.  If its encoding
		 is the same, we can reuse it, unless it is a slice.  */

	      if (memcmp (&src_en, &dst_en, sizeof (ctf_encoding_t)) == 0)
		{
		  if (kind != CTF_K_SLICE)
		    {
		      ctf_add_type_mapping (src_fp, src_type, dst_fp, dst_type);
		      return dst_type;
		    }
		}
	    }
	}
    }

  src.ctb_file = src_fp;
  src.ctb_type = src_type;
  src.ctb_dtd = NULL;

  dst.ctb_file = dst_fp;
  dst.ctb_type = dst_type;
  dst.ctb_dtd = NULL;

  /* Now perform kind-specific processing.  If dst_type is CTF_ERR, then we add
     a new type with the same properties as src_type to dst_fp.  If dst_type is
     not CTF_ERR, then we verify that dst_type has the same attributes as
     src_type.  We recurse for embedded references.  Before we start, we note
     that we are processing this type, to prevent infinite recursion: we do not
     re-process any type that appears in this list.  The list is emptied
     wholesale at the end of processing everything in this recursive stack.  */

  if (ctf_dynhash_insert (proc_tracking_fp->ctf_add_processing,
			  (void *) (uintptr_t) src_type, (void *) 1) < 0)
    return ctf_set_errno (dst_fp, ENOMEM);

  switch (kind)
    {
    case CTF_K_INTEGER:
      /*  If we found a match we will have either returned it or declared a
	  conflict.  */
      dst_type = ctf_add_integer (dst_fp, flag, name, &src_en);
      break;

    case CTF_K_FLOAT:
      /* If we found a match we will have either returned it or declared a
       conflict.  */
      dst_type = ctf_add_float (dst_fp, flag, name, &src_en);
      break;

    case CTF_K_SLICE:
      /* We have checked for conflicting encodings: now try to add the
	 contained type.  */
      src_type = ctf_type_reference (src_fp, src_type);
      src_type = ctf_add_type_internal (dst_fp, src_fp, src_type,
					proc_tracking_fp);

      if (src_type == CTF_ERR)
	return CTF_ERR;				/* errno is set for us.  */

      dst_type = ctf_add_slice (dst_fp, flag, src_type, &src_en);
      break;

    case CTF_K_POINTER:
    case CTF_K_VOLATILE:
    case CTF_K_CONST:
    case CTF_K_RESTRICT:
      src_type = ctf_type_reference (src_fp, src_type);
      src_type = ctf_add_type_internal (dst_fp, src_fp, src_type,
					proc_tracking_fp);

      if (src_type == CTF_ERR)
	return CTF_ERR;				/* errno is set for us.  */

      dst_type = ctf_add_reftype (dst_fp, flag, src_type, kind);
      break;

    case CTF_K_ARRAY:
      if (ctf_array_info (src_fp, src_type, &src_ar) != 0)
	return (ctf_set_errno (dst_fp, ctf_errno (src_fp)));

      src_ar.ctr_contents =
	ctf_add_type_internal (dst_fp, src_fp, src_ar.ctr_contents,
			       proc_tracking_fp);
      src_ar.ctr_index = ctf_add_type_internal (dst_fp, src_fp,
						src_ar.ctr_index,
						proc_tracking_fp);
      src_ar.ctr_nelems = src_ar.ctr_nelems;

      if (src_ar.ctr_contents == CTF_ERR || src_ar.ctr_index == CTF_ERR)
	return CTF_ERR;				/* errno is set for us.  */

      if (dst_type != CTF_ERR)
	{
	  if (ctf_array_info (dst_fp, dst_type, &dst_ar) != 0)
	    return CTF_ERR;			/* errno is set for us.  */

	  if (memcmp (&src_ar, &dst_ar, sizeof (ctf_arinfo_t)))
	    {
	      ctf_dprintf ("Conflict for type %s against ID %lx: "
			   "array info differs, old %lx/%lx/%x; "
			   "new: %lx/%lx/%x\n", name, dst_type,
			   src_ar.ctr_contents, src_ar.ctr_index,
			   src_ar.ctr_nelems, dst_ar.ctr_contents,
			   dst_ar.ctr_index, dst_ar.ctr_nelems);
	      return (ctf_set_errno (dst_fp, ECTF_CONFLICT));
	    }
	}
      else
	dst_type = ctf_add_array (dst_fp, flag, &src_ar);
      break;

    case CTF_K_FUNCTION:
      ctc.ctc_return = ctf_add_type_internal (dst_fp, src_fp,
					      src_tp->ctt_type,
					      proc_tracking_fp);
      ctc.ctc_argc = 0;
      ctc.ctc_flags = 0;

      if (ctc.ctc_return == CTF_ERR)
	return CTF_ERR;				/* errno is set for us.  */

      dst_type = ctf_add_function (dst_fp, flag, &ctc, NULL);
      break;

    case CTF_K_STRUCT:
    case CTF_K_UNION:
      {
	ctf_dmdef_t *dmd;
	int errs = 0;
	size_t size;
	ssize_t ssize;
	ctf_dtdef_t *dtd;

	/* Technically to match a struct or union we need to check both
	   ways (src members vs. dst, dst members vs. src) but we make
	   this more optimal by only checking src vs. dst and comparing
	   the total size of the structure (which we must do anyway)
	   which covers the possibility of dst members not in src.
	   This optimization can be defeated for unions, but is so
	   pathological as to render it irrelevant for our purposes.  */

	if (dst_type != CTF_ERR && kind != CTF_K_FORWARD
	    && dst_kind != CTF_K_FORWARD)
	  {
	    if (ctf_type_size (src_fp, src_type) !=
		ctf_type_size (dst_fp, dst_type))
	      {
		ctf_dprintf ("Conflict for type %s against ID %lx: "
			     "union size differs, old %li, new %li\n",
			     name, dst_type,
			     (long) ctf_type_size (src_fp, src_type),
			     (long) ctf_type_size (dst_fp, dst_type));
		return (ctf_set_errno (dst_fp, ECTF_CONFLICT));
	      }

	    if (ctf_member_iter (src_fp, src_type, membcmp, &dst))
	      {
		ctf_dprintf ("Conflict for type %s against ID %lx: "
			     "members differ, see above\n", name, dst_type);
		return (ctf_set_errno (dst_fp, ECTF_CONFLICT));
	      }

	    break;
	  }

	/* Unlike the other cases, copying structs and unions is done
	   manually so as to avoid repeated lookups in ctf_add_member
	   and to ensure the exact same member offsets as in src_type.  */

	dst_type = ctf_add_generic (dst_fp, flag, name, kind, &dtd);
	if (dst_type == CTF_ERR)
	  return CTF_ERR;			/* errno is set for us.  */

	dst.ctb_type = dst_type;
	dst.ctb_dtd = dtd;

	/* Pre-emptively add this struct to the type mapping so that
	   structures that refer to themselves work.  */
	ctf_add_type_mapping (src_fp, src_type, dst_fp, dst_type);

	if (ctf_member_iter (src_fp, src_type, membadd, &dst) != 0)
	  errs++;	       /* Increment errs and fail at bottom of case.  */

	if ((ssize = ctf_type_size (src_fp, src_type)) < 0)
	  return CTF_ERR;			/* errno is set for us.  */

	size = (size_t) ssize;
	if (size > CTF_MAX_SIZE)
	  {
	    dtd->dtd_data.ctt_size = CTF_LSIZE_SENT;
	    dtd->dtd_data.ctt_lsizehi = CTF_SIZE_TO_LSIZE_HI (size);
	    dtd->dtd_data.ctt_lsizelo = CTF_SIZE_TO_LSIZE_LO (size);
	  }
	else
	  dtd->dtd_data.ctt_size = (uint32_t) size;

	dtd->dtd_data.ctt_info = CTF_TYPE_INFO (kind, flag, vlen);

	/* Make a final pass through the members changing each dmd_type (a
	   src_fp type) to an equivalent type in dst_fp.  We pass through all
	   members, leaving any that fail set to CTF_ERR, unless they fail
	   because they are marking a member of type not representable in this
	   version of CTF, in which case we just want to silently omit them:
	   no consumer can do anything with them anyway.  */
	for (dmd = ctf_list_next (&dtd->dtd_u.dtu_members);
	     dmd != NULL; dmd = ctf_list_next (dmd))
	  {
	    ctf_file_t *dst = dst_fp;
	    ctf_id_t memb_type;

	    memb_type = ctf_type_mapping (src_fp, dmd->dmd_type, &dst);
	    if (memb_type == 0)
	      {
		if ((dmd->dmd_type =
		     ctf_add_type_internal (dst_fp, src_fp, dmd->dmd_type,
					    proc_tracking_fp)) == CTF_ERR)
		  {
		    if (ctf_errno (dst_fp) != ECTF_NONREPRESENTABLE)
		      errs++;
		  }
	      }
	    else
	      dmd->dmd_type = memb_type;
	  }

	if (errs)
	  return CTF_ERR;			/* errno is set for us.  */
	break;
      }

    case CTF_K_ENUM:
      if (dst_type != CTF_ERR && kind != CTF_K_FORWARD
	  && dst_kind != CTF_K_FORWARD)
	{
	  if (ctf_enum_iter (src_fp, src_type, enumcmp, &dst)
	      || ctf_enum_iter (dst_fp, dst_type, enumcmp, &src))
	    {
	      ctf_dprintf ("Conflict for enum %s against ID %lx: "
			   "members differ, see above\n", name, dst_type);
	      return (ctf_set_errno (dst_fp, ECTF_CONFLICT));
	    }
	}
      else
	{
	  dst_type = ctf_add_enum (dst_fp, flag, name);
	  if ((dst.ctb_type = dst_type) == CTF_ERR
	      || ctf_enum_iter (src_fp, src_type, enumadd, &dst))
	    return CTF_ERR;			/* errno is set for us */
	}
      break;

    case CTF_K_FORWARD:
      if (dst_type == CTF_ERR)
	  dst_type = ctf_add_forward (dst_fp, flag, name, forward_kind);
      break;

    case CTF_K_TYPEDEF:
      src_type = ctf_type_reference (src_fp, src_type);
      src_type = ctf_add_type_internal (dst_fp, src_fp, src_type,
					proc_tracking_fp);

      if (src_type == CTF_ERR)
	return CTF_ERR;				/* errno is set for us.  */

      /* If dst_type is not CTF_ERR at this point, we should check if
	 ctf_type_reference(dst_fp, dst_type) != src_type and if so fail with
	 ECTF_CONFLICT.  However, this causes problems with bitness typedefs
	 that vary based on things like if 32-bit then pid_t is int otherwise
	 long.  We therefore omit this check and assume that if the identically
	 named typedef already exists in dst_fp, it is correct or
	 equivalent.  */

      if (dst_type == CTF_ERR)
	  dst_type = ctf_add_typedef (dst_fp, flag, name, src_type);

      break;

    default:
      return (ctf_set_errno (dst_fp, ECTF_CORRUPT));
    }

  if (dst_type != CTF_ERR)
    ctf_add_type_mapping (src_fp, orig_src_type, dst_fp, dst_type);
  return dst_type;
}

ctf_id_t
ctf_add_type (ctf_file_t *dst_fp, ctf_file_t *src_fp, ctf_id_t src_type)
{
  ctf_id_t id;

  if (!src_fp->ctf_add_processing)
    src_fp->ctf_add_processing = ctf_dynhash_create (ctf_hash_integer,
						     ctf_hash_eq_integer,
						     NULL, NULL);

  /* We store the hash on the source, because it contains only source type IDs:
     but callers will invariably expect errors to appear on the dest.  */
  if (!src_fp->ctf_add_processing)
    return (ctf_set_errno (dst_fp, ENOMEM));

  id = ctf_add_type_internal (dst_fp, src_fp, src_type, src_fp);
  ctf_dynhash_empty (src_fp->ctf_add_processing);

  return id;
}

/* Write the compressed CTF data stream to the specified gzFile descriptor.  */
int
ctf_gzwrite (ctf_file_t *fp, gzFile fd)
{
  const unsigned char *buf;
  ssize_t resid;
  ssize_t len;

  resid = sizeof (ctf_header_t);
  buf = (unsigned char *) fp->ctf_header;
  while (resid != 0)
    {
      if ((len = gzwrite (fd, buf, resid)) <= 0)
	return (ctf_set_errno (fp, errno));
      resid -= len;
      buf += len;
    }

  resid = fp->ctf_size;
  buf = fp->ctf_buf;
  while (resid != 0)
    {
      if ((len = gzwrite (fd, buf, resid)) <= 0)
	return (ctf_set_errno (fp, errno));
      resid -= len;
      buf += len;
    }

  return 0;
}

/* Compress the specified CTF data stream and write it to the specified file
   descriptor.  */
int
ctf_compress_write (ctf_file_t *fp, int fd)
{
  unsigned char *buf;
  unsigned char *bp;
  ctf_header_t h;
  ctf_header_t *hp = &h;
  ssize_t header_len = sizeof (ctf_header_t);
  ssize_t compress_len;
  ssize_t len;
  int rc;
  int err = 0;

  if (ctf_serialize (fp) < 0)
    return -1;					/* errno is set for us.  */

  memcpy (hp, fp->ctf_header, header_len);
  hp->cth_flags |= CTF_F_COMPRESS;
  compress_len = compressBound (fp->ctf_size);

  if ((buf = malloc (compress_len)) == NULL)
    return (ctf_set_errno (fp, ECTF_ZALLOC));

  if ((rc = compress (buf, (uLongf *) &compress_len,
		      fp->ctf_buf, fp->ctf_size)) != Z_OK)
    {
      ctf_dprintf ("zlib deflate err: %s\n", zError (rc));
      err = ctf_set_errno (fp, ECTF_COMPRESS);
      goto ret;
    }

  while (header_len > 0)
    {
      if ((len = write (fd, hp, header_len)) < 0)
	{
	  err = ctf_set_errno (fp, errno);
	  goto ret;
	}
      header_len -= len;
      hp += len;
    }

  bp = buf;
  while (compress_len > 0)
    {
      if ((len = write (fd, bp, compress_len)) < 0)
	{
	  err = ctf_set_errno (fp, errno);
	  goto ret;
	}
      compress_len -= len;
      bp += len;
    }

ret:
  free (buf);
  return err;
}

/* Optionally compress the specified CTF data stream and return it as a new
   dynamically-allocated string.  */
unsigned char *
ctf_write_mem (ctf_file_t *fp, size_t *size, size_t threshold)
{
  unsigned char *buf;
  unsigned char *bp;
  ctf_header_t *hp;
  ssize_t header_len = sizeof (ctf_header_t);
  ssize_t compress_len;
  int rc;

  if (ctf_serialize (fp) < 0)
    return NULL;				/* errno is set for us.  */

  compress_len = compressBound (fp->ctf_size);
  if (fp->ctf_size < threshold)
    compress_len = fp->ctf_size;
  if ((buf = malloc (compress_len
		     + sizeof (struct ctf_header))) == NULL)
    {
      ctf_set_errno (fp, ENOMEM);
      return NULL;
    }

  hp = (ctf_header_t *) buf;
  memcpy (hp, fp->ctf_header, header_len);
  bp = buf + sizeof (struct ctf_header);
  *size = sizeof (struct ctf_header);

  if (fp->ctf_size < threshold)
    {
      hp->cth_flags &= ~CTF_F_COMPRESS;
      memcpy (bp, fp->ctf_buf, fp->ctf_size);
      *size += fp->ctf_size;
    }
  else
    {
      hp->cth_flags |= CTF_F_COMPRESS;
      if ((rc = compress (bp, (uLongf *) &compress_len,
			  fp->ctf_buf, fp->ctf_size)) != Z_OK)
	{
	  ctf_dprintf ("zlib deflate err: %s\n", zError (rc));
	  ctf_set_errno (fp, ECTF_COMPRESS);
	  free (buf);
	  return NULL;
	}
      *size += compress_len;
    }
  return buf;
}

/* Write the uncompressed CTF data stream to the specified file descriptor.  */
int
ctf_write (ctf_file_t *fp, int fd)
{
  const unsigned char *buf;
  ssize_t resid;
  ssize_t len;

  if (ctf_serialize (fp) < 0)
    return -1;					/* errno is set for us.  */

  resid = sizeof (ctf_header_t);
  buf = (unsigned char *) fp->ctf_header;
  while (resid != 0)
    {
      if ((len = write (fd, buf, resid)) <= 0)
	return (ctf_set_errno (fp, errno));
      resid -= len;
      buf += len;
    }

  resid = fp->ctf_size;
  buf = fp->ctf_buf;
  while (resid != 0)
    {
      if ((len = write (fd, buf, resid)) <= 0)
	return (ctf_set_errno (fp, errno));
      resid -= len;
      buf += len;
    }

  return 0;
}
