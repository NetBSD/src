/* LoongArch-specific support for NN-bit ELF.
   Copyright (C) 2021-2022 Free Software Foundation, Inc.
   Contributed by Loongson Ltd.

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
   along with this program; see the file COPYING3.  If not,
   see <http://www.gnu.org/licenses/>.  */

#include "ansidecl.h"
#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#define ARCH_SIZE NN
#include "elf-bfd.h"
#include "objalloc.h"
#include "elf/loongarch.h"
#include "elfxx-loongarch.h"

static bool
loongarch_info_to_howto_rela (bfd *abfd, arelent *cache_ptr,
			      Elf_Internal_Rela *dst)
{
  cache_ptr->howto = loongarch_elf_rtype_to_howto (abfd,
						   ELFNN_R_TYPE (dst->r_info));
  return cache_ptr->howto != NULL;
}

/* LoongArch ELF linker hash entry.  */
struct loongarch_elf_link_hash_entry
{
  struct elf_link_hash_entry elf;

#define GOT_UNKNOWN 0
#define GOT_NORMAL  1
#define GOT_TLS_GD  2
#define GOT_TLS_IE  4
#define GOT_TLS_LE  8
  char tls_type;
};

#define loongarch_elf_hash_entry(ent)	\
  ((struct loongarch_elf_link_hash_entry *) (ent))

struct _bfd_loongarch_elf_obj_tdata
{
  struct elf_obj_tdata root;

  /* The tls_type for each local got entry.  */
  char *local_got_tls_type;
};

#define _bfd_loongarch_elf_tdata(abfd)	\
  ((struct _bfd_loongarch_elf_obj_tdata *) (abfd)->tdata.any)

#define _bfd_loongarch_elf_local_got_tls_type(abfd)	\
  (_bfd_loongarch_elf_tdata (abfd)->local_got_tls_type)

#define _bfd_loongarch_elf_tls_type(abfd, h, symndx)			\
  (*((h) != NULL							\
     ? &loongarch_elf_hash_entry (h)->tls_type				\
     : &_bfd_loongarch_elf_local_got_tls_type (abfd)[symndx]))

#define is_loongarch_elf(bfd)						\
  (bfd_get_flavour (bfd) == bfd_target_elf_flavour			\
   && elf_tdata (bfd) != NULL						\
   && elf_object_id (bfd) == LARCH_ELF_DATA)

struct loongarch_elf_link_hash_table
{
  struct elf_link_hash_table elf;

  /* Short-cuts to get to dynamic linker sections.  */
  asection *sdyntdata;

  /* Small local sym to section mapping cache.  */
  struct sym_cache sym_cache;

  /* Used by local STT_GNU_IFUNC symbols.  */
  htab_t loc_hash_table;
  void *loc_hash_memory;

  /* The max alignment of output sections.  */
  bfd_vma max_alignment;
};

/* Get the LoongArch ELF linker hash table from a link_info structure.  */
#define loongarch_elf_hash_table(p)					\
  (elf_hash_table_id (elf_hash_table (p)) == LARCH_ELF_DATA		\
   ? ((struct loongarch_elf_link_hash_table *) ((p)->hash))		\
   : NULL)

#define MINUS_ONE ((bfd_vma) 0 - 1)

#define sec_addr(sec) ((sec)->output_section->vma + (sec)->output_offset)

#define LARCH_ELF_LOG_WORD_BYTES (ARCH_SIZE == 32 ? 2 : 3)
#define LARCH_ELF_WORD_BYTES (1 << LARCH_ELF_LOG_WORD_BYTES)

#define PLT_HEADER_INSNS 8
#define PLT_HEADER_SIZE (PLT_HEADER_INSNS * 4)

#define PLT_ENTRY_INSNS 4
#define PLT_ENTRY_SIZE (PLT_ENTRY_INSNS * 4)

#define GOT_ENTRY_SIZE (LARCH_ELF_WORD_BYTES)

#define GOTPLT_HEADER_SIZE (GOT_ENTRY_SIZE * 2)

#define elf_backend_want_got_plt 1

#define elf_backend_plt_readonly 1

#define elf_backend_want_plt_sym 0
#define elf_backend_plt_alignment 4
#define elf_backend_can_gc_sections 1
#define elf_backend_want_got_sym 1

#define elf_backend_got_header_size (GOT_ENTRY_SIZE * 1)

#define elf_backend_want_dynrelro 1

/* Generate a PLT header.  */

static bool
loongarch_make_plt_header (bfd_vma got_plt_addr, bfd_vma plt_header_addr,
			   uint32_t *entry)
{
  bfd_vma pcrel = got_plt_addr - plt_header_addr;
  bfd_vma hi, lo;

  if (pcrel + 0x80000800 > 0xffffffff)
    {
      _bfd_error_handler (_("%#" PRIx64 " invaild imm"), (uint64_t) pcrel);
      bfd_set_error (bfd_error_bad_value);
      return false;
    }
  hi = ((pcrel + 0x800) >> 12) & 0xfffff;
  lo = pcrel & 0xfff;

  /* pcaddu12i  $t2, %hi(%pcrel(.got.plt))
     sub.[wd]   $t1, $t1, $t3
     ld.[wd]    $t3, $t2, %lo(%pcrel(.got.plt)) # _dl_runtime_resolve
     addi.[wd]  $t1, $t1, -(PLT_HEADER_SIZE + 12)
     addi.[wd]  $t0, $t2, %lo(%pcrel(.got.plt))
     srli.[wd]  $t1, $t1, log2(16 / GOT_ENTRY_SIZE)
     ld.[wd]    $t0, $t0, GOT_ENTRY_SIZE
     jirl   $r0, $t3, 0 */

  if (GOT_ENTRY_SIZE == 8)
    {
      entry[0] = 0x1c00000e | (hi & 0xfffff) << 5;
      entry[1] = 0x0011bdad;
      entry[2] = 0x28c001cf | (lo & 0xfff) << 10;
      entry[3] = 0x02c001ad | ((-(PLT_HEADER_SIZE + 12)) & 0xfff) << 10;
      entry[4] = 0x02c001cc | (lo & 0xfff) << 10;
      entry[5] = 0x004501ad | (4 - LARCH_ELF_LOG_WORD_BYTES) << 10;
      entry[6] = 0x28c0018c | GOT_ENTRY_SIZE << 10;
      entry[7] = 0x4c0001e0;
    }
  else
    {
      entry[0] = 0x1c00000e | (hi & 0xfffff) << 5;
      entry[1] = 0x00113dad;
      entry[2] = 0x288001cf | (lo & 0xfff) << 10;
      entry[3] = 0x028001ad | ((-(PLT_HEADER_SIZE + 12)) & 0xfff) << 10;
      entry[4] = 0x028001cc | (lo & 0xfff) << 10;
      entry[5] = 0x004481ad | (4 - LARCH_ELF_LOG_WORD_BYTES) << 10;
      entry[6] = 0x2880018c | GOT_ENTRY_SIZE << 10;
      entry[7] = 0x4c0001e0;
    }
  return true;
}

/* Generate a PLT entry.  */

static bool
loongarch_make_plt_entry (bfd_vma got_plt_entry_addr, bfd_vma plt_entry_addr,
			  uint32_t *entry)
{
  bfd_vma pcrel = got_plt_entry_addr - plt_entry_addr;
  bfd_vma hi, lo;

  if (pcrel + 0x80000800 > 0xffffffff)
    {
      _bfd_error_handler (_("%#" PRIx64 " invaild imm"), (uint64_t) pcrel);
      bfd_set_error (bfd_error_bad_value);
      return false;
    }
  hi = ((pcrel + 0x800) >> 12) & 0xfffff;
  lo = pcrel & 0xfff;

  entry[0] = 0x1c00000f | (hi & 0xfffff) << 5;
  entry[1] = ((GOT_ENTRY_SIZE == 8 ? 0x28c001ef : 0x288001ef)
	      | (lo & 0xfff) << 10);
  entry[2] = 0x4c0001ed;	/* jirl $r13, $15, 0 */
  entry[3] = 0x03400000;	/* nop */

  return true;
}

/* Create an entry in an LoongArch ELF linker hash table.  */

static struct bfd_hash_entry *
link_hash_newfunc (struct bfd_hash_entry *entry, struct bfd_hash_table *table,
		   const char *string)
{
  struct loongarch_elf_link_hash_entry *eh;

  /* Allocate the structure if it has not already been allocated by a
     subclass.  */
  if (entry == NULL)
    {
      entry = bfd_hash_allocate (table, sizeof (*eh));
      if (entry == NULL)
	return entry;
    }

  /* Call the allocation method of the superclass.  */
  entry = _bfd_elf_link_hash_newfunc (entry, table, string);
  if (entry != NULL)
    {
      eh = (struct loongarch_elf_link_hash_entry *) entry;
      eh->tls_type = GOT_UNKNOWN;
    }

  return entry;
}

/* Compute a hash of a local hash entry.  We use elf_link_hash_entry
  for local symbol so that we can handle local STT_GNU_IFUNC symbols
  as global symbol.  We reuse indx and dynstr_index for local symbol
  hash since they aren't used by global symbols in this backend.  */

static hashval_t
elfNN_loongarch_local_htab_hash (const void *ptr)
{
  struct elf_link_hash_entry *h = (struct elf_link_hash_entry *) ptr;
  return ELF_LOCAL_SYMBOL_HASH (h->indx, h->dynstr_index);
}

/* Compare local hash entries.  */

static int
elfNN_loongarch_local_htab_eq (const void *ptr1, const void *ptr2)
{
  struct elf_link_hash_entry *h1 = (struct elf_link_hash_entry *) ptr1;
  struct elf_link_hash_entry *h2 = (struct elf_link_hash_entry *) ptr2;

  return h1->indx == h2->indx && h1->dynstr_index == h2->dynstr_index;
}

/* Find and/or create a hash entry for local symbol.  */
static struct elf_link_hash_entry *
elfNN_loongarch_get_local_sym_hash (struct loongarch_elf_link_hash_table *htab,
				    bfd *abfd, const Elf_Internal_Rela *rel,
				    bool create)
{
  struct loongarch_elf_link_hash_entry e, *ret;
  asection *sec = abfd->sections;
  hashval_t h = ELF_LOCAL_SYMBOL_HASH (sec->id, ELFNN_R_SYM (rel->r_info));
  void **slot;

  e.elf.indx = sec->id;
  e.elf.dynstr_index = ELFNN_R_SYM (rel->r_info);
  slot = htab_find_slot_with_hash (htab->loc_hash_table, &e, h,
				   create ? INSERT : NO_INSERT);

  if (!slot)
    return NULL;

  if (*slot)
    {
      ret = (struct loongarch_elf_link_hash_entry *) *slot;
      return &ret->elf;
    }

  ret = ((struct loongarch_elf_link_hash_entry *)
	 objalloc_alloc ((struct objalloc *) htab->loc_hash_memory,
			 sizeof (struct loongarch_elf_link_hash_entry)));
  if (ret)
    {
      memset (ret, 0, sizeof (*ret));
      ret->elf.indx = sec->id;
      ret->elf.pointer_equality_needed = 0;
      ret->elf.dynstr_index = ELFNN_R_SYM (rel->r_info);
      ret->elf.dynindx = -1;
      ret->elf.needs_plt = 0;
      ret->elf.plt.refcount = -1;
      ret->elf.got.refcount = -1;
      ret->elf.def_dynamic = 0;
      ret->elf.def_regular = 1;
      ret->elf.ref_dynamic = 0; /* This should be always 0 for local.  */
      ret->elf.ref_regular = 0;
      ret->elf.forced_local = 1;
      ret->elf.root.type = bfd_link_hash_defined;
      *slot = ret;
    }
  return &ret->elf;
}

/* Destroy an LoongArch elf linker hash table.  */

static void
elfNN_loongarch_link_hash_table_free (bfd *obfd)
{
  struct loongarch_elf_link_hash_table *ret;
  ret = (struct loongarch_elf_link_hash_table *) obfd->link.hash;

  if (ret->loc_hash_table)
    htab_delete (ret->loc_hash_table);
  if (ret->loc_hash_memory)
    objalloc_free ((struct objalloc *) ret->loc_hash_memory);

  _bfd_elf_link_hash_table_free (obfd);
}

/* Create a LoongArch ELF linker hash table.  */

static struct bfd_link_hash_table *
loongarch_elf_link_hash_table_create (bfd *abfd)
{
  struct loongarch_elf_link_hash_table *ret;
  bfd_size_type amt = sizeof (struct loongarch_elf_link_hash_table);

  ret = (struct loongarch_elf_link_hash_table *) bfd_zmalloc (amt);
  if (ret == NULL)
    return NULL;

  if (!_bfd_elf_link_hash_table_init
      (&ret->elf, abfd, link_hash_newfunc,
       sizeof (struct loongarch_elf_link_hash_entry), LARCH_ELF_DATA))
    {
      free (ret);
      return NULL;
    }

  ret->max_alignment = MINUS_ONE;

  ret->loc_hash_table = htab_try_create (1024, elfNN_loongarch_local_htab_hash,
					 elfNN_loongarch_local_htab_eq, NULL);
  ret->loc_hash_memory = objalloc_create ();
  if (!ret->loc_hash_table || !ret->loc_hash_memory)
    {
      elfNN_loongarch_link_hash_table_free (abfd);
      return NULL;
    }
  ret->elf.root.hash_table_free = elfNN_loongarch_link_hash_table_free;

  return &ret->elf.root;
}

/* Merge backend specific data from an object file to the output
   object file when linking.  */

static bool
elfNN_loongarch_merge_private_bfd_data (bfd *ibfd, struct bfd_link_info *info)
{
  bfd *obfd = info->output_bfd;
  flagword in_flags = elf_elfheader (ibfd)->e_flags;
  flagword out_flags = elf_elfheader (obfd)->e_flags;

  if (!is_loongarch_elf (ibfd) || !is_loongarch_elf (obfd))
    return true;

  if (strcmp (bfd_get_target (ibfd), bfd_get_target (obfd)) != 0)
    {
      _bfd_error_handler (_("%pB: ABI is incompatible with that of "
			    "the selected emulation:\n"
			    "  target emulation `%s' does not match `%s'"),
			  ibfd, bfd_get_target (ibfd), bfd_get_target (obfd));
      return false;
    }

  if (!_bfd_elf_merge_object_attributes (ibfd, info))
    return false;

  /* If the input BFD is not a dynamic object and it does not contain any
     non-data sections, do not account its ABI.  For example, various
     packages produces such data-only relocatable objects with
     `ld -r -b binary` or `objcopy`, and these objects have zero e_flags.
     But they are compatible with all ABIs.  */
  if (!(ibfd->flags & DYNAMIC))
    {
      asection *sec;
      bool have_code_sections = false;
      for (sec = ibfd->sections; sec != NULL; sec = sec->next)
	if ((bfd_section_flags (sec)
	     & (SEC_LOAD | SEC_CODE | SEC_HAS_CONTENTS))
	    == (SEC_LOAD | SEC_CODE | SEC_HAS_CONTENTS))
	  {
	    have_code_sections = true;
	    break;
	  }
      if (!have_code_sections)
	return true;
    }

  if (!elf_flags_init (obfd))
    {
      elf_flags_init (obfd) = true;
      elf_elfheader (obfd)->e_flags = in_flags;
      return true;
    }

  /* Disallow linking different ABIs.  */
  if (EF_LOONGARCH_ABI(out_flags ^ in_flags) & EF_LOONGARCH_ABI_MASK)
    {
      _bfd_error_handler (_("%pB: can't link different ABI object."), ibfd);
      goto fail;
    }

  return true;

 fail:
  bfd_set_error (bfd_error_bad_value);
  return false;
}

/* Create the .got section.  */

static bool
loongarch_elf_create_got_section (bfd *abfd, struct bfd_link_info *info)
{
  flagword flags;
  char *name;
  asection *s, *s_got;
  struct elf_link_hash_entry *h;
  const struct elf_backend_data *bed = get_elf_backend_data (abfd);
  struct elf_link_hash_table *htab = elf_hash_table (info);

  /* This function may be called more than once.  */
  if (htab->sgot != NULL)
    return true;

  flags = bed->dynamic_sec_flags;
  name = bed->rela_plts_and_copies_p ? ".rela.got" : ".rel.got";
  s = bfd_make_section_anyway_with_flags (abfd, name, flags | SEC_READONLY);

  if (s == NULL || !bfd_set_section_alignment (s, bed->s->log_file_align))
    return false;
  htab->srelgot = s;

  s = s_got = bfd_make_section_anyway_with_flags (abfd, ".got", flags);
  if (s == NULL || !bfd_set_section_alignment (s, bed->s->log_file_align))
    return false;
  htab->sgot = s;

  /* The first bit of the global offset table is the header.  */
  s->size += bed->got_header_size;

  if (bed->want_got_plt)
    {
      s = bfd_make_section_anyway_with_flags (abfd, ".got.plt", flags);
      if (s == NULL || !bfd_set_section_alignment (s, bed->s->log_file_align))
	return false;
      htab->sgotplt = s;

      /* Reserve room for the header.  */
      s->size = GOTPLT_HEADER_SIZE;
    }

  if (bed->want_got_sym)
    {
      /* Define the symbol _GLOBAL_OFFSET_TABLE_ at the start of the .got
	 section.  We don't do this in the linker script because we don't want
	 to define the symbol if we are not creating a global offset table.  */
      h = _bfd_elf_define_linkage_sym (abfd, info, s_got,
				       "_GLOBAL_OFFSET_TABLE_");
      elf_hash_table (info)->hgot = h;
      if (h == NULL)
	return false;
    }
  return true;
}

/* Create .plt, .rela.plt, .got, .got.plt, .rela.got, .dynbss, and
   .rela.bss sections in DYNOBJ, and set up shortcuts to them in our
   hash table.  */

static bool
loongarch_elf_create_dynamic_sections (bfd *dynobj, struct bfd_link_info *info)
{
  struct loongarch_elf_link_hash_table *htab;

  htab = loongarch_elf_hash_table (info);
  BFD_ASSERT (htab != NULL);

  if (!loongarch_elf_create_got_section (dynobj, info))
    return false;

  if (!_bfd_elf_create_dynamic_sections (dynobj, info))
    return false;

  if (!bfd_link_pic (info))
    htab->sdyntdata
      = bfd_make_section_anyway_with_flags (dynobj, ".tdata.dyn",
					    SEC_ALLOC | SEC_THREAD_LOCAL);

  if (!htab->elf.splt || !htab->elf.srelplt || !htab->elf.sdynbss
      || (!bfd_link_pic (info) && (!htab->elf.srelbss || !htab->sdyntdata)))
    abort ();

  return true;
}

static bool
loongarch_elf_record_tls_and_got_reference (bfd *abfd,
					    struct bfd_link_info *info,
					    struct elf_link_hash_entry *h,
					    unsigned long symndx,
					    char tls_type)
{
  struct loongarch_elf_link_hash_table *htab = loongarch_elf_hash_table (info);
  Elf_Internal_Shdr *symtab_hdr = &elf_tdata (abfd)->symtab_hdr;

  /* This is a global offset table entry for a local symbol.  */
  if (elf_local_got_refcounts (abfd) == NULL)
    {
      bfd_size_type size =
	symtab_hdr->sh_info * (sizeof (bfd_vma) + sizeof (tls_type));
      if (!(elf_local_got_refcounts (abfd) = bfd_zalloc (abfd, size)))
	return false;
      _bfd_loongarch_elf_local_got_tls_type (abfd) =
	(char *) (elf_local_got_refcounts (abfd) + symtab_hdr->sh_info);
    }

  switch (tls_type)
    {
    case GOT_NORMAL:
    case GOT_TLS_GD:
    case GOT_TLS_IE:
      /* Need GOT.  */
      if (htab->elf.sgot == NULL
	  && !loongarch_elf_create_got_section (htab->elf.dynobj, info))
	return false;
      if (h)
	{
	  if (h->got.refcount < 0)
	    h->got.refcount = 0;
	  h->got.refcount++;
	}
      else
	elf_local_got_refcounts (abfd)[symndx]++;
      break;
    case GOT_TLS_LE:
      /* No need for GOT.  */
      break;
    default:
      _bfd_error_handler (_("Internal error: unreachable."));
      return false;
    }

  char *new_tls_type = &_bfd_loongarch_elf_tls_type (abfd, h, symndx);
  *new_tls_type |= tls_type;
  if ((*new_tls_type & GOT_NORMAL) && (*new_tls_type & ~GOT_NORMAL))
    {
      _bfd_error_handler (_("%pB: `%s' accessed both as normal and "
			    "thread local symbol"),
			  abfd,
			  h ? h->root.root.string : "<local>");
      return false;
    }

  return true;
}

/* Look through the relocs for a section during the first phase, and
   allocate space in the global offset table or procedure linkage
   table.  */

static bool
loongarch_elf_check_relocs (bfd *abfd, struct bfd_link_info *info,
			    asection *sec, const Elf_Internal_Rela *relocs)
{
  struct loongarch_elf_link_hash_table *htab;
  Elf_Internal_Shdr *symtab_hdr;
  struct elf_link_hash_entry **sym_hashes;
  const Elf_Internal_Rela *rel;
  asection *sreloc = NULL;

  if (bfd_link_relocatable (info))
    return true;

  htab = loongarch_elf_hash_table (info);
  symtab_hdr = &elf_tdata (abfd)->symtab_hdr;
  sym_hashes = elf_sym_hashes (abfd);

  if (htab->elf.dynobj == NULL)
    htab->elf.dynobj = abfd;

  for (rel = relocs; rel < relocs + sec->reloc_count; rel++)
    {
      unsigned int r_type;
      unsigned int r_symndx;
      struct elf_link_hash_entry *h;
      Elf_Internal_Sym *isym = NULL;

      int need_dynreloc;
      int only_need_pcrel;

      r_symndx = ELFNN_R_SYM (rel->r_info);
      r_type = ELFNN_R_TYPE (rel->r_info);

      if (r_symndx >= NUM_SHDR_ENTRIES (symtab_hdr))
	{
	  _bfd_error_handler (_("%pB: bad symbol index: %d"), abfd, r_symndx);
	  return false;
	}

      if (r_symndx < symtab_hdr->sh_info)
	{
	  /* A local symbol.  */
	  isym = bfd_sym_from_r_symndx (&htab->sym_cache, abfd, r_symndx);
	  if (isym == NULL)
	    return false;

	  if (ELF_ST_TYPE (isym->st_info) == STT_GNU_IFUNC)
	    {
	      h = elfNN_loongarch_get_local_sym_hash (htab, abfd, rel, true);
	      if (h == NULL)
		return false;

	      h->type = STT_GNU_IFUNC;
	      h->ref_regular = 1;
	    }
	  else
	    h = NULL;
	}
      else
	{
	  h = sym_hashes[r_symndx - symtab_hdr->sh_info];
	  while (h->root.type == bfd_link_hash_indirect
		 || h->root.type == bfd_link_hash_warning)
	    h = (struct elf_link_hash_entry *) h->root.u.i.link;
	}

      /* It is referenced by a non-shared object.  */
      if (h != NULL)
	h->ref_regular = 1;

      if (h && h->type == STT_GNU_IFUNC)
	{
	  if (htab->elf.dynobj == NULL)
	    htab->elf.dynobj = abfd;

	  /* Create the ifunc sections, iplt and ipltgot, for static
	     executables.  */
	  if ((r_type == R_LARCH_64 || r_type == R_LARCH_32)
	      && !_bfd_elf_create_ifunc_sections (htab->elf.dynobj, info))
	    return false;

	  if (!htab->elf.splt
	      && !_bfd_elf_create_ifunc_sections (htab->elf.dynobj, info))
	    /* If '.plt' not represent, create '.iplt' to deal with ifunc.  */
	    return false;

	  if (h->plt.refcount < 0)
	    h->plt.refcount = 0;
	  h->plt.refcount++;
	  h->needs_plt = 1;

	  elf_tdata (info->output_bfd)->has_gnu_osabi |= elf_gnu_osabi_ifunc;
	}

      need_dynreloc = 0;
      only_need_pcrel = 0;
      switch (r_type)
	{
	case R_LARCH_SOP_PUSH_GPREL:
	  if (!loongarch_elf_record_tls_and_got_reference (abfd, info, h,
							   r_symndx,
							   GOT_NORMAL))
	    return false;
	  break;

	case R_LARCH_SOP_PUSH_TLS_GD:
	  if (!loongarch_elf_record_tls_and_got_reference (abfd, info, h,
							   r_symndx,
							   GOT_TLS_GD))
	    return false;
	  break;

	case R_LARCH_SOP_PUSH_TLS_GOT:
	  if (bfd_link_pic (info))
	    /* May fail for lazy-bind.  */
	    info->flags |= DF_STATIC_TLS;

	  if (!loongarch_elf_record_tls_and_got_reference (abfd, info, h,
							   r_symndx,
							   GOT_TLS_IE))
	    return false;
	  break;

	case R_LARCH_SOP_PUSH_TLS_TPREL:
	  if (!bfd_link_executable (info))
	    return false;

	  info->flags |= DF_STATIC_TLS;

	  if (!loongarch_elf_record_tls_and_got_reference (abfd, info, h,
							   r_symndx,
							   GOT_TLS_LE))
	    return false;
	  break;

	case R_LARCH_SOP_PUSH_ABSOLUTE:
	  if (h != NULL)
	    /* If this reloc is in a read-only section, we might
	       need a copy reloc.  We can't check reliably at this
	       stage whether the section is read-only, as input
	       sections have not yet been mapped to output sections.
	       Tentatively set the flag for now, and correct in
	       adjust_dynamic_symbol.  */
	    h->non_got_ref = 1;
	  break;

	case R_LARCH_SOP_PUSH_PCREL:
	  if (h != NULL)
	    {
	      h->non_got_ref = 1;

	      /* We try to create PLT stub for all non-local function.  */
	      if (h->plt.refcount < 0)
		h->plt.refcount = 0;
	      h->plt.refcount++;
	    }
	  break;

	case R_LARCH_SOP_PUSH_PLT_PCREL:
	  /* This symbol requires a procedure linkage table entry.  We
	     actually build the entry in adjust_dynamic_symbol,
	     because this might be a case of linking PIC code without
	     linking in any dynamic objects, in which case we don't
	     need to generate a procedure linkage table after all.  */
	  if (h != NULL)
	    {
	      h->needs_plt = 1;
	      if (h->plt.refcount < 0)
		h->plt.refcount = 0;
	      h->plt.refcount++;
	    }
	  break;

	case R_LARCH_TLS_DTPREL32:
	case R_LARCH_TLS_DTPREL64:
	  need_dynreloc = 1;
	  only_need_pcrel = 1;
	  break;

	case R_LARCH_JUMP_SLOT:
	case R_LARCH_32:
	case R_LARCH_64:
	  need_dynreloc = 1;

	  /* If resolved symbol is defined in this object,
	     1. Under pie, the symbol is known.  We convert it
	     into R_LARCH_RELATIVE and need load-addr still.
	     2. Under pde, the symbol is known and we can discard R_LARCH_NN.
	     3. Under dll, R_LARCH_NN can't be changed normally, since
	     its defination could be covered by the one in executable.
	     For symbolic, we convert it into R_LARCH_RELATIVE.
	     Thus, only under pde, it needs pcrel only.  We discard it.  */
	  only_need_pcrel = bfd_link_pde (info);

	  if (h != NULL)
	    h->non_got_ref = 1;

	  if (h != NULL
	      && (!bfd_link_pic (info)
		  || h->type == STT_GNU_IFUNC))
	    {
	      /* This reloc might not bind locally.  */
	      h->non_got_ref = 1;
	      h->pointer_equality_needed = 1;

	      if (!h->def_regular
		  || (sec->flags & (SEC_CODE | SEC_READONLY)) != 0)
		{
		  /* We may need a .plt entry if the symbol is a function
		     defined in a shared lib or is a function referenced
		     from the code or read-only section.  */
		  h->plt.refcount += 1;
		}
	    }
	  break;

	case R_LARCH_GNU_VTINHERIT:
	  if (!bfd_elf_gc_record_vtinherit (abfd, sec, h, rel->r_offset))
	    return false;
	  break;

	case R_LARCH_GNU_VTENTRY:
	  if (!bfd_elf_gc_record_vtentry (abfd, sec, h, rel->r_addend))
	    return false;
	  break;

	default:
	  break;
	}

      /* Record some info for sizing and allocating dynamic entry.  */
      if (need_dynreloc && (sec->flags & SEC_ALLOC))
	{
	  /* When creating a shared object, we must copy these
	     relocs into the output file.  We create a reloc
	     section in dynobj and make room for the reloc.  */
	  struct elf_dyn_relocs *p;
	  struct elf_dyn_relocs **head;

	  if (sreloc == NULL)
	    {
	      sreloc
		= _bfd_elf_make_dynamic_reloc_section (sec, htab->elf.dynobj,
						       LARCH_ELF_LOG_WORD_BYTES,
						       abfd, /*rela?*/ true);
	      if (sreloc == NULL)
		return false;
	    }

	  /* If this is a global symbol, we count the number of
	     relocations we need for this symbol.  */
	  if (h != NULL)
	    head = &h->dyn_relocs;
	  else
	    {
	      /* Track dynamic relocs needed for local syms too.
		 We really need local syms available to do this
		 easily.  Oh well.  */

	      asection *s;
	      void *vpp;

	      s = bfd_section_from_elf_index (abfd, isym->st_shndx);
	      if (s == NULL)
		s = sec;

	      vpp = &elf_section_data (s)->local_dynrel;
	      head = (struct elf_dyn_relocs **) vpp;
	    }

	  p = *head;
	  if (p == NULL || p->sec != sec)
	    {
	      bfd_size_type amt = sizeof *p;
	      p = (struct elf_dyn_relocs *) bfd_alloc (htab->elf.dynobj, amt);
	      if (p == NULL)
		return false;
	      p->next = *head;
	      *head = p;
	      p->sec = sec;
	      p->count = 0;
	      p->pc_count = 0;
	    }

	  p->count++;
	  p->pc_count += only_need_pcrel;
	}
    }

  return true;
}

/* Find dynamic relocs for H that apply to read-only sections.  */

static asection *
readonly_dynrelocs (struct elf_link_hash_entry *h)
{
  struct elf_dyn_relocs *p;

  for (p = h->dyn_relocs; p != NULL; p = p->next)
    {
      asection *s = p->sec->output_section;

      if (s != NULL && (s->flags & SEC_READONLY) != 0)
	return p->sec;
    }
  return NULL;
}

/* Adjust a symbol defined by a dynamic object and referenced by a
   regular object.  The current definition is in some section of the
   dynamic object, but we're not including those sections.  We have to
   change the definition to something the rest of the link can
   understand.  */
static bool
loongarch_elf_adjust_dynamic_symbol (struct bfd_link_info *info,
				     struct elf_link_hash_entry *h)
{
  struct loongarch_elf_link_hash_table *htab;
  struct loongarch_elf_link_hash_entry *eh;
  bfd *dynobj;
  asection *s, *srel;

  htab = loongarch_elf_hash_table (info);
  BFD_ASSERT (htab != NULL);

  dynobj = htab->elf.dynobj;

  /* Make sure we know what is going on here.  */
  BFD_ASSERT (dynobj != NULL
	      && (h->needs_plt || h->type == STT_GNU_IFUNC || h->is_weakalias
		  || (h->def_dynamic && h->ref_regular && !h->def_regular)));

  /* If this is a function, put it in the procedure linkage table.  We
     will fill in the contents of the procedure linkage table later
     (although we could actually do it here).  */
  if (h->type == STT_FUNC || h->type == STT_GNU_IFUNC || h->needs_plt)
    {
      if (h->plt.refcount < 0
	  || (h->type != STT_GNU_IFUNC
	      && (SYMBOL_REFERENCES_LOCAL (info, h)
		  || (ELF_ST_VISIBILITY (h->other) != STV_DEFAULT
		      && h->root.type == bfd_link_hash_undefweak))))
	{
	  /* This case can occur if we saw a R_LARCH_SOP_PUSH_PLT_PCREL reloc
	     in an input file, but the symbol was never referred to by a
	     dynamic object, or if all references were garbage collected.
	     In such a case, we don't actually need to build a PLT entry.  */
	  h->plt.offset = MINUS_ONE;
	  h->needs_plt = 0;
	}
      else
	h->needs_plt = 1;

      return true;
    }
  else
    h->plt.offset = MINUS_ONE;

  /* If this is a weak symbol, and there is a real definition, the
     processor independent code will have arranged for us to see the
     real definition first, and we can just use the same value.  */
  if (h->is_weakalias)
    {
      struct elf_link_hash_entry *def = weakdef (h);
      BFD_ASSERT (def->root.type == bfd_link_hash_defined);
      h->root.u.def.section = def->root.u.def.section;
      h->root.u.def.value = def->root.u.def.value;
      return true;
    }

  /* This is a reference to a symbol defined by a dynamic object which
     is not a function.  */

  /* If we are creating a shared library, we must presume that the
     only references to the symbol are via the global offset table.
     For such cases we need not do anything here; the relocations will
     be handled correctly by relocate_section.  */
  if (bfd_link_dll (info))
    return true;

  /* If there are no references to this symbol that do not use the
     GOT, we don't need to generate a copy reloc.  */
  if (!h->non_got_ref)
    return true;

  /* If -z nocopyreloc was given, we won't generate them either.  */
  if (info->nocopyreloc)
    {
      h->non_got_ref = 0;
      return true;
    }

  /* If we don't find any dynamic relocs in read-only sections, then
     we'll be keeping the dynamic relocs and avoiding the copy reloc.  */
  if (!readonly_dynrelocs (h))
    {
      h->non_got_ref = 0;
      return true;
    }

  /* We must allocate the symbol in our .dynbss section, which will
     become part of the .bss section of the executable.  There will be
     an entry for this symbol in the .dynsym section.  The dynamic
     object will contain position independent code, so all references
     from the dynamic object to this symbol will go through the global
     offset table.  The dynamic linker will use the .dynsym entry to
     determine the address it must put in the global offset table, so
     both the dynamic object and the regular object will refer to the
     same memory location for the variable.  */

  /* We must generate a R_LARCH_COPY reloc to tell the dynamic linker
     to copy the initial value out of the dynamic object and into the
     runtime process image.  We need to remember the offset into the
     .rel.bss section we are going to use.  */
  eh = (struct loongarch_elf_link_hash_entry *) h;
  if (eh->tls_type & ~GOT_NORMAL)
    {
      s = htab->sdyntdata;
      srel = htab->elf.srelbss;
    }
  else if ((h->root.u.def.section->flags & SEC_READONLY) != 0)
    {
      s = htab->elf.sdynrelro;
      srel = htab->elf.sreldynrelro;
    }
  else
    {
      s = htab->elf.sdynbss;
      srel = htab->elf.srelbss;
    }
  if ((h->root.u.def.section->flags & SEC_ALLOC) != 0 && h->size != 0)
    {
      srel->size += sizeof (ElfNN_External_Rela);
      h->needs_copy = 1;
    }

  return _bfd_elf_adjust_dynamic_copy (info, h, s);
}

/* Allocate space in .plt, .got and associated reloc sections for
   dynamic relocs.  */

static bool
allocate_dynrelocs (struct elf_link_hash_entry *h, void *inf)
{
  struct bfd_link_info *info;
  struct loongarch_elf_link_hash_table *htab;
  struct elf_dyn_relocs *p;

  if (h->root.type == bfd_link_hash_indirect)
    return true;

  if (h->type == STT_GNU_IFUNC
      && h->def_regular)
    return true;

  info = (struct bfd_link_info *) inf;
  htab = loongarch_elf_hash_table (info);
  BFD_ASSERT (htab != NULL);

  do
    {
      asection *plt, *gotplt, *relplt;

      if (!h->needs_plt)
	break;

      h->needs_plt = 0;

      if (htab->elf.splt)
	{
	  if (h->dynindx == -1 && !h->forced_local
	      && !bfd_elf_link_record_dynamic_symbol (info, h))
	    return false;

	  if (!WILL_CALL_FINISH_DYNAMIC_SYMBOL (1, bfd_link_pic (info), h)
	      && h->type != STT_GNU_IFUNC)
	    break;

	  plt = htab->elf.splt;
	  gotplt = htab->elf.sgotplt;
	  relplt = htab->elf.srelplt;
	}
      else if (htab->elf.iplt)
	{
	  /* .iplt only for IFUNC.  */
	  if (h->type != STT_GNU_IFUNC)
	    break;

	  plt = htab->elf.iplt;
	  gotplt = htab->elf.igotplt;
	  relplt = htab->elf.irelplt;
	}
      else
	break;

      if (plt->size == 0)
	plt->size = PLT_HEADER_SIZE;

      h->plt.offset = plt->size;
      plt->size += PLT_ENTRY_SIZE;
      gotplt->size += GOT_ENTRY_SIZE;
      relplt->size += sizeof (ElfNN_External_Rela);

      /* If this symbol is not defined in a regular file, and we are
	 not generating a shared library, then set the symbol to this
	 location in the .plt.  This is required to make function
	 pointers compare as equal between the normal executable and
	 the shared library.  */
      if (!bfd_link_pic(info)
	  && !h->def_regular)
	{
	  h->root.u.def.section = plt;
	  h->root.u.def.value = h->plt.offset;
	}

      h->needs_plt = 1;
    }
  while (0);

  if (!h->needs_plt)
    h->plt.offset = MINUS_ONE;

  if (0 < h->got.refcount)
    {
      asection *s;
      bool dyn;
      int tls_type = loongarch_elf_hash_entry (h)->tls_type;

      /* Make sure this symbol is output as a dynamic symbol.
	 Undefined weak syms won't yet be marked as dynamic.  */
      if (h->dynindx == -1 && !h->forced_local)
	{
	  if (SYMBOL_REFERENCES_LOCAL (info, h)
	      && (ELF_ST_VISIBILITY (h->other) != STV_DEFAULT)
	      && h->start_stop)
	    {
	      /* The pr21964-4. do nothing.  */
	    }
	  else
	    {
	      if( !bfd_elf_link_record_dynamic_symbol (info, h))
		return false;
	    }
	}

      s = htab->elf.sgot;
      h->got.offset = s->size;
      dyn = htab->elf.dynamic_sections_created;
      if (tls_type & (GOT_TLS_GD | GOT_TLS_IE))
	{
	  /* TLS_GD needs two dynamic relocs and two GOT slots.  */
	  if (tls_type & GOT_TLS_GD)
	    {
	      s->size += 2 * GOT_ENTRY_SIZE;
	      htab->elf.srelgot->size += 2 * sizeof (ElfNN_External_Rela);
	    }

	  /* TLS_IE needs one dynamic reloc and one GOT slot.  */
	  if (tls_type & GOT_TLS_IE)
	    {
	      s->size += GOT_ENTRY_SIZE;
	      htab->elf.srelgot->size += sizeof (ElfNN_External_Rela);
	    }
	}
      else
	{
	  s->size += GOT_ENTRY_SIZE;
	  if ((WILL_CALL_FINISH_DYNAMIC_SYMBOL (dyn, bfd_link_pic (info), h)
	       && !UNDEFWEAK_NO_DYNAMIC_RELOC (info, h))
	      || h->type == STT_GNU_IFUNC)
	    htab->elf.srelgot->size += sizeof (ElfNN_External_Rela);
	}
    }
  else
    h->got.offset = MINUS_ONE;

  if (h->dyn_relocs == NULL)
    return true;

  if (SYMBOL_REFERENCES_LOCAL (info, h))
    {
      struct elf_dyn_relocs **pp;

      for (pp = &h->dyn_relocs; (p = *pp) != NULL;)
	{
	  p->count -= p->pc_count;
	  p->pc_count = 0;
	  if (p->count == 0)
	    *pp = p->next;
	  else
	    pp = &p->next;
	}
    }

  if (h->root.type == bfd_link_hash_undefweak)
    {
      if (UNDEFWEAK_NO_DYNAMIC_RELOC (info, h))
	h->dyn_relocs = NULL;
      else if (h->dynindx == -1 && !h->forced_local
	       /* Make sure this symbol is output as a dynamic symbol.
		  Undefined weak syms won't yet be marked as dynamic.  */
	       && !bfd_elf_link_record_dynamic_symbol (info, h))
	return false;
    }

  for (p = h->dyn_relocs; p != NULL; p = p->next)
    {
      asection *sreloc = elf_section_data (p->sec)->sreloc;
      sreloc->size += p->count * sizeof (ElfNN_External_Rela);
    }

  return true;
}

/* Allocate space in .plt, .got and associated reloc sections for
   ifunc dynamic relocs.  */

static bool
elfNN_loongarch_allocate_ifunc_dynrelocs (struct elf_link_hash_entry *h,
					  void *inf)
{
  struct bfd_link_info *info;
  /* An example of a bfd_link_hash_indirect symbol is versioned
     symbol. For example: __gxx_personality_v0(bfd_link_hash_indirect)
     -> __gxx_personality_v0(bfd_link_hash_defined)

     There is no need to process bfd_link_hash_indirect symbols here
     because we will also be presented with the concrete instance of
     the symbol and loongarch_elf_copy_indirect_symbol () will have been
     called to copy all relevant data from the generic to the concrete
     symbol instance.  */
  if (h->root.type == bfd_link_hash_indirect)
    return true;

  if (h->root.type == bfd_link_hash_warning)
    h = (struct elf_link_hash_entry *) h->root.u.i.link;

  info = (struct bfd_link_info *) inf;

  /* Since STT_GNU_IFUNC symbol must go through PLT, we handle it
     here if it is defined and referenced in a non-shared object.  */
  if (h->type == STT_GNU_IFUNC
      && h->def_regular)
    return _bfd_elf_allocate_ifunc_dyn_relocs (info, h,
					       &h->dyn_relocs,
					       PLT_ENTRY_SIZE,
					       PLT_HEADER_SIZE,
					       GOT_ENTRY_SIZE,
					       false);
  return true;
}

/* Allocate space in .plt, .got and associated reloc sections for
   ifunc dynamic relocs.  */

static bool
elfNN_loongarch_allocate_local_dynrelocs (void **slot, void *inf)
{
  struct elf_link_hash_entry *h = (struct elf_link_hash_entry *) *slot;

  if (h->type != STT_GNU_IFUNC
      || !h->def_regular
      || !h->ref_regular
      || !h->forced_local
      || h->root.type != bfd_link_hash_defined)
    abort ();

  return elfNN_loongarch_allocate_ifunc_dynrelocs (h, inf);
}

/* Set DF_TEXTREL if we find any dynamic relocs that apply to
   read-only sections.  */

static bool
maybe_set_textrel (struct elf_link_hash_entry *h, void *info_p)
{
  asection *sec;

  if (h->root.type == bfd_link_hash_indirect)
    return true;

  sec = readonly_dynrelocs (h);
  if (sec != NULL)
    {
      struct bfd_link_info *info = (struct bfd_link_info *) info_p;

      info->flags |= DF_TEXTREL;
      info->callbacks->minfo (_("%pB: dynamic relocation against `%pT' in "
				"read-only section `%pA'\n"),
			      sec->owner, h->root.root.string, sec);

      /* Not an error, just cut short the traversal.  */
      return false;
    }
  return true;
}

static bool
loongarch_elf_size_dynamic_sections (bfd *output_bfd,
				     struct bfd_link_info *info)
{
  struct loongarch_elf_link_hash_table *htab;
  bfd *dynobj;
  asection *s;
  bfd *ibfd;

  htab = loongarch_elf_hash_table (info);
  BFD_ASSERT (htab != NULL);
  dynobj = htab->elf.dynobj;
  BFD_ASSERT (dynobj != NULL);

  if (htab->elf.dynamic_sections_created)
    {
      /* Set the contents of the .interp section to the interpreter.  */
      if (bfd_link_executable (info) && !info->nointerp)
	{
	  const char *interpreter;
	  flagword flags = elf_elfheader (output_bfd)->e_flags;
	  s = bfd_get_linker_section (dynobj, ".interp");
	  BFD_ASSERT (s != NULL);
	  if (EF_LOONGARCH_IS_ILP32 (flags))
	    interpreter = "/lib32/ld.so.1";
	  else if (EF_LOONGARCH_IS_LP64 (flags))
	    interpreter = "/lib64/ld.so.1";
	  else
	    interpreter = "/lib/ld.so.1";
	  s->contents = (unsigned char *) interpreter;
	  s->size = strlen (interpreter) + 1;
	}
    }

  /* Set up .got offsets for local syms, and space for local dynamic
     relocs.  */
  for (ibfd = info->input_bfds; ibfd != NULL; ibfd = ibfd->link.next)
    {
      bfd_signed_vma *local_got;
      bfd_signed_vma *end_local_got;
      char *local_tls_type;
      bfd_size_type locsymcount;
      Elf_Internal_Shdr *symtab_hdr;
      asection *srel;

      if (!is_loongarch_elf (ibfd))
	continue;

      for (s = ibfd->sections; s != NULL; s = s->next)
	{
	  struct elf_dyn_relocs *p;

	  for (p = elf_section_data (s)->local_dynrel; p != NULL; p = p->next)
	    {
	      p->count -= p->pc_count;
	      if (!bfd_is_abs_section (p->sec)
		  && bfd_is_abs_section (p->sec->output_section))
		{
		  /* Input section has been discarded, either because
		     it is a copy of a linkonce section or due to
		     linker script /DISCARD/, so we'll be discarding
		     the relocs too.  */
		}
	      else if (0 < p->count)
		{
		  srel = elf_section_data (p->sec)->sreloc;
		  srel->size += p->count * sizeof (ElfNN_External_Rela);
		  if ((p->sec->output_section->flags & SEC_READONLY) != 0)
		    info->flags |= DF_TEXTREL;
		}
	    }
	}

      local_got = elf_local_got_refcounts (ibfd);
      if (!local_got)
	continue;

      symtab_hdr = &elf_symtab_hdr (ibfd);
      locsymcount = symtab_hdr->sh_info;
      end_local_got = local_got + locsymcount;
      local_tls_type = _bfd_loongarch_elf_local_got_tls_type (ibfd);
      s = htab->elf.sgot;
      srel = htab->elf.srelgot;
      for (; local_got < end_local_got; ++local_got, ++local_tls_type)
	{
	  if (0 < *local_got)
	    {
	      *local_got = s->size;
	      s->size += GOT_ENTRY_SIZE;

	      if (*local_tls_type & GOT_TLS_GD)
		s->size += GOT_ENTRY_SIZE;

	      /* If R_LARCH_RELATIVE.  */
	      if (bfd_link_pic (info)
		  /* Or R_LARCH_TLS_DTPRELNN or R_LARCH_TLS_TPRELNN.  */
		  || (*local_tls_type & (GOT_TLS_GD | GOT_TLS_IE)))
		srel->size += sizeof (ElfNN_External_Rela);
	    }
	  else
	    *local_got = MINUS_ONE;
	}
    }

  /* Allocate global sym .plt and .got entries, and space for global
     sym dynamic relocs.  */
  elf_link_hash_traverse (&htab->elf, allocate_dynrelocs, info);

  /* Allocate global ifunc sym .plt and .got entries, and space for global
     ifunc sym dynamic relocs.  */
  elf_link_hash_traverse (&htab->elf, elfNN_loongarch_allocate_ifunc_dynrelocs, info);

  /* Allocate .plt and .got entries, and space for local ifunc symbols.  */
  htab_traverse (htab->loc_hash_table,
		 (void *) elfNN_loongarch_allocate_local_dynrelocs, info);

  /* Don't allocate .got.plt section if there are no PLT.  */
  if (htab->elf.sgotplt && htab->elf.sgotplt->size == GOTPLT_HEADER_SIZE
      && (htab->elf.splt == NULL || htab->elf.splt->size == 0))
    htab->elf.sgotplt->size = 0;

  /* The check_relocs and adjust_dynamic_symbol entry points have
     determined the sizes of the various dynamic sections.  Allocate
     memory for them.  */
  for (s = dynobj->sections; s != NULL; s = s->next)
    {
      if ((s->flags & SEC_LINKER_CREATED) == 0)
	continue;

      if (s == htab->elf.splt || s == htab->elf.iplt || s == htab->elf.sgot
	  || s == htab->elf.sgotplt || s == htab->elf.igotplt
	  || s == htab->elf.sdynbss || s == htab->elf.sdynrelro)
	{
	  /* Strip this section if we don't need it; see the
	     comment below.  */
	}
      else if (strncmp (s->name, ".rela", 5) == 0)
	{
	  if (s->size != 0)
	    {
	      /* We use the reloc_count field as a counter if we need
		 to copy relocs into the output file.  */
	      s->reloc_count = 0;
	    }
	}
      else
	{
	  /* It's not one of our sections.  */
	  continue;
	}

      if (s->size == 0)
	{
	  /* If we don't need this section, strip it from the
	     output file.  This is mostly to handle .rela.bss and
	     .rela.plt.  We must create both sections in
	     create_dynamic_sections, because they must be created
	     before the linker maps input sections to output
	     sections.  The linker does that before
	     adjust_dynamic_symbol is called, and it is that
	     function which decides whether anything needs to go
	     into these sections.  */
	  s->flags |= SEC_EXCLUDE;
	  continue;
	}

      if ((s->flags & SEC_HAS_CONTENTS) == 0)
	continue;

      /* Allocate memory for the section contents.  Zero the memory
	 for the benefit of .rela.plt, which has 4 unused entries
	 at the beginning, and we don't want garbage.  */
      s->contents = (bfd_byte *) bfd_zalloc (dynobj, s->size);
      if (s->contents == NULL)
	return false;
    }

  if (elf_hash_table (info)->dynamic_sections_created)
    {
      /* Add some entries to the .dynamic section.  We fill in the
	 values later, in loongarch_elf_finish_dynamic_sections, but we
	 must add the entries now so that we get the correct size for
	 the .dynamic section.  The DT_DEBUG entry is filled in by the
	 dynamic linker and used by the debugger.  */
#define add_dynamic_entry(TAG, VAL) _bfd_elf_add_dynamic_entry (info, TAG, VAL)

      if (bfd_link_executable (info))
	{
	  if (!add_dynamic_entry (DT_DEBUG, 0))
	    return false;
	}

      if (htab->elf.srelplt->size != 0)
	{
	  if (!add_dynamic_entry (DT_PLTGOT, 0)
	      || !add_dynamic_entry (DT_PLTRELSZ, 0)
	      || !add_dynamic_entry (DT_PLTREL, DT_RELA)
	      || !add_dynamic_entry (DT_JMPREL, 0))
	    return false;
	}

      if (!add_dynamic_entry (DT_RELA, 0)
	  || !add_dynamic_entry (DT_RELASZ, 0)
	  || !add_dynamic_entry (DT_RELAENT, sizeof (ElfNN_External_Rela)))
	return false;

      /* If any dynamic relocs apply to a read-only section,
	 then we need a DT_TEXTREL entry.  */
      if ((info->flags & DF_TEXTREL) == 0)
	elf_link_hash_traverse (&htab->elf, maybe_set_textrel, info);

      if (info->flags & DF_TEXTREL)
	{
	  if (!add_dynamic_entry (DT_TEXTREL, 0))
	    return false;
	  /* Clear the DF_TEXTREL flag.  It will be set again if we
	     write out an actual text relocation; we may not, because
	     at this point we do not know whether e.g.  any .eh_frame
	     absolute relocations have been converted to PC-relative.  */
	  info->flags &= ~DF_TEXTREL;
	}
    }
#undef add_dynamic_entry

  return true;
}

#define LARCH_LD_STACK_DEPTH 16
static int64_t larch_opc_stack[LARCH_LD_STACK_DEPTH];
static size_t larch_stack_top = 0;

static bfd_reloc_status_type
loongarch_push (int64_t val)
{
  if (LARCH_LD_STACK_DEPTH <= larch_stack_top)
    return bfd_reloc_outofrange;
  larch_opc_stack[larch_stack_top++] = val;
  return bfd_reloc_ok;
}

static bfd_reloc_status_type
loongarch_pop (int64_t *val)
{
  if (larch_stack_top == 0)
    return bfd_reloc_outofrange;
  BFD_ASSERT (val);
  *val = larch_opc_stack[--larch_stack_top];
  return bfd_reloc_ok;
}

static bfd_reloc_status_type
loongarch_top (int64_t *val)
{
  if (larch_stack_top == 0)
    return bfd_reloc_outofrange;
  BFD_ASSERT (val);
  *val = larch_opc_stack[larch_stack_top - 1];
  return bfd_reloc_ok;
}

static void
loongarch_elf_append_rela (bfd *abfd, asection *s, Elf_Internal_Rela *rel)
{
  const struct elf_backend_data *bed;
  bfd_byte *loc;

  bed = get_elf_backend_data (abfd);
  loc = s->contents + (s->reloc_count++ * bed->s->sizeof_rela);
  bed->s->swap_reloca_out (abfd, rel, loc);
}

/* Check rel->r_offset in range of contents.  */
static bfd_reloc_status_type
loongarch_check_offset (const Elf_Internal_Rela *rel,
			const asection *input_section)
{
  if (0 == strcmp(input_section->name, ".text")
      && rel->r_offset > input_section->size)
    return bfd_reloc_overflow;

  return bfd_reloc_ok;
}

#define LARCH_RELOC_PERFORM_3OP(op1, op2, op3)	      \
  ({						      \
    bfd_reloc_status_type ret = loongarch_pop (&op2); \
    if (ret == bfd_reloc_ok)			      \
      {						      \
	ret = loongarch_pop (&op1);		      \
	if (ret == bfd_reloc_ok)		      \
	  ret = loongarch_push (op3);		      \
      }						      \
    ret;					      \
   })

static bfd_reloc_status_type
loongarch_reloc_rewrite_imm_insn (const Elf_Internal_Rela *rel,
				  const asection *input_section ATTRIBUTE_UNUSED,
				  reloc_howto_type *howto, bfd *input_bfd,
				  bfd_byte *contents, bfd_vma reloc_val)
{
  int bits = bfd_get_reloc_size (howto) * 8;
  uint32_t insn = bfd_get (bits, input_bfd, contents + rel->r_offset);

  if (!loongarch_adjust_reloc_bitsfield(howto, &reloc_val))
    return bfd_reloc_overflow;

  insn = (insn & (uint32_t)howto->src_mask)
    | ((insn & (~(uint32_t)howto->dst_mask)) | reloc_val);

  bfd_put (bits, input_bfd, insn, contents + rel->r_offset);

  return bfd_reloc_ok;
}

/* Emplace a static relocation.  */

static bfd_reloc_status_type
perform_relocation (const Elf_Internal_Rela *rel, asection *input_section,
		    reloc_howto_type *howto, bfd_vma value,
		    bfd *input_bfd, bfd_byte *contents)
{
  uint32_t insn1;
  int64_t opr1, opr2, opr3;
  bfd_reloc_status_type r = bfd_reloc_ok;
  int bits = bfd_get_reloc_size (howto) * 8;


  switch (ELFNN_R_TYPE (rel->r_info))
    {
    case R_LARCH_SOP_PUSH_PCREL:
    case R_LARCH_SOP_PUSH_ABSOLUTE:
    case R_LARCH_SOP_PUSH_GPREL:
    case R_LARCH_SOP_PUSH_TLS_TPREL:
    case R_LARCH_SOP_PUSH_TLS_GOT:
    case R_LARCH_SOP_PUSH_TLS_GD:
    case R_LARCH_SOP_PUSH_PLT_PCREL:
      r = loongarch_push (value);
      break;

    case R_LARCH_SOP_PUSH_DUP:
      r = loongarch_pop (&opr1);
      if (r == bfd_reloc_ok)
	{
	  r = loongarch_push (opr1);
	  if (r == bfd_reloc_ok)
	    r = loongarch_push (opr1);
	}
      break;

    case R_LARCH_SOP_ASSERT:
      r = loongarch_pop (&opr1);
      if (r != bfd_reloc_ok || !opr1)
	r = bfd_reloc_notsupported;
      break;

    case R_LARCH_SOP_NOT:
      r = loongarch_pop (&opr1);
      if (r == bfd_reloc_ok)
	r = loongarch_push (!opr1);
      break;

    case R_LARCH_SOP_SUB:
      r = LARCH_RELOC_PERFORM_3OP (opr1, opr2, opr1 - opr2);
      break;

    case R_LARCH_SOP_SL:
      r = LARCH_RELOC_PERFORM_3OP (opr1, opr2, opr1 << opr2);
      break;

    case R_LARCH_SOP_SR:
      r = LARCH_RELOC_PERFORM_3OP (opr1, opr2, opr1 >> opr2);
      break;

    case R_LARCH_SOP_AND:
      r = LARCH_RELOC_PERFORM_3OP (opr1, opr2, opr1 & opr2);
      break;

    case R_LARCH_SOP_ADD:
      r = LARCH_RELOC_PERFORM_3OP (opr1, opr2, opr1 + opr2);
      break;

    case R_LARCH_SOP_IF_ELSE:
      r = loongarch_pop (&opr3);
      if (r == bfd_reloc_ok)
	{
	  r = loongarch_pop (&opr2);
	  if (r == bfd_reloc_ok)
	    {
	      r = loongarch_pop (&opr1);
	      if (r == bfd_reloc_ok)
		r = loongarch_push (opr1 ? opr2 : opr3);
	    }
	}
      break;

    case R_LARCH_SOP_POP_32_S_10_5:
    case R_LARCH_SOP_POP_32_S_10_12:
    case R_LARCH_SOP_POP_32_S_10_16:
    case R_LARCH_SOP_POP_32_S_10_16_S2:
    case R_LARCH_SOP_POP_32_S_5_20:
    case R_LARCH_SOP_POP_32_U_10_12:
    case R_LARCH_SOP_POP_32_U:
      r = loongarch_pop (&opr1);
      if (r != bfd_reloc_ok)
	break;
      r = loongarch_check_offset (rel, input_section);
      if (r != bfd_reloc_ok)
	break;

      r = loongarch_reloc_rewrite_imm_insn (rel, input_section,
					    howto, input_bfd,
					    contents, (bfd_vma)opr1);
      break;

    case R_LARCH_SOP_POP_32_S_0_5_10_16_S2:
	{
	  r = loongarch_pop (&opr1);
	  if (r != bfd_reloc_ok)
	    break;

	  if ((opr1 & 0x3) != 0)
	    {
	      r = bfd_reloc_overflow;
	      break;
	    }

	  uint32_t imm = opr1 >> howto->rightshift;
	  if ((imm & (~0xfffffU)) && ((imm & (~0xfffffU)) != (~0xfffffU)))
	    {
	      r = bfd_reloc_overflow;
	      break;
	    }
	  r = loongarch_check_offset (rel, input_section);
	  if (r != bfd_reloc_ok)
	    break;

	  insn1 = bfd_get (bits, input_bfd, contents + rel->r_offset);
	  insn1 = (insn1 & howto->src_mask)
	    | ((imm & 0xffffU) << 10)
	    | ((imm & 0x1f0000U) >> 16);
	  bfd_put (bits, input_bfd, insn1, contents + rel->r_offset);
	  break;
	}

    case R_LARCH_SOP_POP_32_S_0_10_10_16_S2:
      {
	r = loongarch_pop (&opr1);
	if (r != bfd_reloc_ok)
	  break;

	if ((opr1 & 0x3) != 0)
	  {
	    r = bfd_reloc_overflow;
	    break;
	  }

	uint32_t imm = opr1 >> howto->rightshift;
	if ((imm & (~0x1ffffffU)) && (imm & (~0x1ffffffU)) != (~0x1ffffffU))
	  {
	    r = bfd_reloc_overflow;
	    break;
	  }

	r = loongarch_check_offset (rel, input_section);
	if (r != bfd_reloc_ok)
	  break;

	insn1 = bfd_get (bits, input_bfd, contents + rel->r_offset);
	insn1 = ((insn1 & howto->src_mask)
		 | ((imm & 0xffffU) << 10)
		 | ((imm & 0x3ff0000U) >> 16));
	bfd_put (bits, input_bfd, insn1, contents + rel->r_offset);
	break;
      }

    case R_LARCH_TLS_DTPREL32:
    case R_LARCH_32:
    case R_LARCH_TLS_DTPREL64:
    case R_LARCH_64:
      r = loongarch_check_offset (rel, input_section);
      if (r != bfd_reloc_ok)
	break;

      bfd_put (bits, input_bfd, value, contents + rel->r_offset);
      break;

    case R_LARCH_ADD8:
    case R_LARCH_ADD16:
    case R_LARCH_ADD24:
    case R_LARCH_ADD32:
    case R_LARCH_ADD64:
      r = loongarch_check_offset (rel, input_section);
      if (r != bfd_reloc_ok)
	break;

      opr1 = bfd_get (bits, input_bfd, contents + rel->r_offset);
      bfd_put (bits, input_bfd, opr1 + value, contents + rel->r_offset);
      break;

    case R_LARCH_SUB8:
    case R_LARCH_SUB16:
    case R_LARCH_SUB24:
    case R_LARCH_SUB32:
    case R_LARCH_SUB64:
      r = loongarch_check_offset (rel, input_section);
      if (r != bfd_reloc_ok)
	break;

      opr1 = bfd_get (bits, input_bfd, contents + rel->r_offset);
      bfd_put (bits, input_bfd, opr1 - value, contents + rel->r_offset);
      break;

    default:
      r = bfd_reloc_notsupported;
    }
  return r;
}

#define LARCH_RECENT_RELOC_QUEUE_LENGTH 72
static struct
{
  bfd *bfd;
  asection *section;
  bfd_vma r_offset;
  int r_type;
  bfd_vma relocation;
  Elf_Internal_Sym *sym;
  struct elf_link_hash_entry *h;
  bfd_vma addend;
  int64_t top_then;
} larch_reloc_queue[LARCH_RECENT_RELOC_QUEUE_LENGTH];
static size_t larch_reloc_queue_head = 0;
static size_t larch_reloc_queue_tail = 0;

static const char *
loongarch_sym_name (bfd *input_bfd, struct elf_link_hash_entry *h,
		    Elf_Internal_Sym *sym)
{
  const char *ret = NULL;
  if (sym)
    ret = bfd_elf_string_from_elf_section (input_bfd,
					   elf_symtab_hdr (input_bfd).sh_link,
					   sym->st_name);
  else if (h)
    ret = h->root.root.string;

  if (ret == NULL || *ret == '\0')
    ret = "<nameless>";
  return ret;
}

static void
loongarch_record_one_reloc (bfd *abfd, asection *section, int r_type,
			    bfd_vma r_offset, Elf_Internal_Sym *sym,
			    struct elf_link_hash_entry *h, bfd_vma addend)
{
  if ((larch_reloc_queue_head == 0
       && larch_reloc_queue_tail == LARCH_RECENT_RELOC_QUEUE_LENGTH - 1)
      || larch_reloc_queue_head == larch_reloc_queue_tail + 1)
    larch_reloc_queue_head =
      (larch_reloc_queue_head + 1) % LARCH_RECENT_RELOC_QUEUE_LENGTH;
  larch_reloc_queue[larch_reloc_queue_tail].bfd = abfd;
  larch_reloc_queue[larch_reloc_queue_tail].section = section;
  larch_reloc_queue[larch_reloc_queue_tail].r_offset = r_offset;
  larch_reloc_queue[larch_reloc_queue_tail].r_type = r_type;
  larch_reloc_queue[larch_reloc_queue_tail].sym = sym;
  larch_reloc_queue[larch_reloc_queue_tail].h = h;
  larch_reloc_queue[larch_reloc_queue_tail].addend = addend;
  loongarch_top (&larch_reloc_queue[larch_reloc_queue_tail].top_then);
  larch_reloc_queue_tail =
    (larch_reloc_queue_tail + 1) % LARCH_RECENT_RELOC_QUEUE_LENGTH;
}

static void
loongarch_dump_reloc_record (void (*p) (const char *fmt, ...))
{
  size_t i = larch_reloc_queue_head;
  bfd *a_bfd = NULL;
  asection *section = NULL;
  bfd_vma r_offset = 0;
  int inited = 0;
  p ("Dump relocate record:\n");
  p ("stack top\t\trelocation name\t\tsymbol");
  while (i != larch_reloc_queue_tail)
    {
      if (a_bfd != larch_reloc_queue[i].bfd
	  || section != larch_reloc_queue[i].section
	  || r_offset != larch_reloc_queue[i].r_offset)
	{
	  a_bfd = larch_reloc_queue[i].bfd;
	  section = larch_reloc_queue[i].section;
	  r_offset = larch_reloc_queue[i].r_offset;
	  p ("\nat %pB(%pA+0x%v):\n", larch_reloc_queue[i].bfd,
	     larch_reloc_queue[i].section, larch_reloc_queue[i].r_offset);
	}

      if (!inited)
	inited = 1, p ("...\n");

      reloc_howto_type *howto =
	loongarch_elf_rtype_to_howto (larch_reloc_queue[i].bfd,
				      larch_reloc_queue[i].r_type);
      p ("0x%V %s\t`%s'", (bfd_vma) larch_reloc_queue[i].top_then,
	 howto ? howto->name : "<unknown reloc>",
	 loongarch_sym_name (larch_reloc_queue[i].bfd, larch_reloc_queue[i].h,
			     larch_reloc_queue[i].sym));

      long addend = larch_reloc_queue[i].addend;
      if (addend < 0)
	p (" - %ld", -addend);
      else if (0 < addend)
	p (" + %ld(0x%v)", addend, larch_reloc_queue[i].addend);

      p ("\n");
      i = (i + 1) % LARCH_RECENT_RELOC_QUEUE_LENGTH;
    }
  p ("\n"
     "-- Record dump end --\n\n");
}


static bool
loongarch_reloc_is_fatal (struct bfd_link_info *info,
			  bfd *input_bfd,
			  asection *input_section,
			  Elf_Internal_Rela *rel,
			  reloc_howto_type *howto,
			  bfd_reloc_status_type rtype,
			  bool is_undefweak,
			  const char *name,
			  const char *msg)
{
  bool fatal = true;
  switch (rtype)
    {
      /* 'dangerous' means we do it but can't promise it's ok
	 'unsupport' means out of ability of relocation type
	 'undefined' means we can't deal with the undefined symbol.  */
    case bfd_reloc_undefined:
      info->callbacks->undefined_symbol (info, name, input_bfd, input_section,
					 rel->r_offset, true);
      info->callbacks->info ("%X%pB(%pA+0x%v): error: %s against %s`%s':\n%s\n",
			     input_bfd, input_section, rel->r_offset,
			     howto->name,
			     is_undefweak ? "[undefweak] " : "", name, msg);
      break;
    case bfd_reloc_dangerous:
      info->callbacks->info ("%pB(%pA+0x%v): warning: %s against %s`%s':\n%s\n",
			     input_bfd, input_section, rel->r_offset,
			     howto->name,
			     is_undefweak ? "[undefweak] " : "", name, msg);
      fatal = false;
      break;
    case bfd_reloc_notsupported:
      info->callbacks->info ("%X%pB(%pA+0x%v): error: %s against %s`%s':\n%s\n",
			     input_bfd, input_section, rel->r_offset,
			     howto->name,
			     is_undefweak ? "[undefweak] " : "", name, msg);
      break;
    default:
      break;
    }
  return fatal;
}




static int
loongarch_elf_relocate_section (bfd *output_bfd, struct bfd_link_info *info,
				bfd *input_bfd, asection *input_section,
				bfd_byte *contents, Elf_Internal_Rela *relocs,
				Elf_Internal_Sym *local_syms,
				asection **local_sections)
{
  Elf_Internal_Rela *rel;
  Elf_Internal_Rela *relend;
  bool fatal = false;
  asection *sreloc = elf_section_data (input_section)->sreloc;
  struct loongarch_elf_link_hash_table *htab = loongarch_elf_hash_table (info);
  Elf_Internal_Shdr *symtab_hdr = &elf_symtab_hdr (input_bfd);
  struct elf_link_hash_entry **sym_hashes = elf_sym_hashes (input_bfd);
  bfd_vma *local_got_offsets = elf_local_got_offsets (input_bfd);
  bool is_pic = bfd_link_pic (info);
  bool is_dyn = elf_hash_table (info)->dynamic_sections_created;
  asection *plt = htab->elf.splt ? htab->elf.splt : htab->elf.iplt;
  asection *got = htab->elf.sgot;

  relend = relocs + input_section->reloc_count;
  for (rel = relocs; rel < relend; rel++)
    {
      int r_type = ELFNN_R_TYPE (rel->r_info);
      unsigned long r_symndx = ELFNN_R_SYM (rel->r_info);
      bfd_vma pc = sec_addr (input_section) + rel->r_offset;
      reloc_howto_type *howto = NULL;
      asection *sec = NULL;
      Elf_Internal_Sym *sym = NULL;
      struct elf_link_hash_entry *h = NULL;
      const char *name;
      bfd_reloc_status_type r = bfd_reloc_ok;
      bool is_ie, is_undefweak, unresolved_reloc, defined_local;
      bool resolved_local, resolved_dynly, resolved_to_const;
      char tls_type;
      bfd_vma relocation;
      bfd_vma off, ie_off;
      int i, j;

      howto = loongarch_elf_rtype_to_howto (input_bfd, r_type);
      if (howto == NULL || r_type == R_LARCH_GNU_VTINHERIT
	  || r_type == R_LARCH_GNU_VTENTRY)
	continue;

      /* This is a final link.  */
      if (r_symndx < symtab_hdr->sh_info)
	{
	  is_undefweak = false;
	  unresolved_reloc = false;
	  sym = local_syms + r_symndx;
	  sec = local_sections[r_symndx];
	  relocation = _bfd_elf_rela_local_sym (output_bfd, sym, &sec, rel);

	  /* Relocate against local STT_GNU_IFUNC symbol.  */
	  if (!bfd_link_relocatable (info)
	      && ELF_ST_TYPE (sym->st_info) == STT_GNU_IFUNC)
	    {
	      h = elfNN_loongarch_get_local_sym_hash (htab, input_bfd, rel,
						      false);
	      if (h == NULL)
		abort ();

	      /* Set STT_GNU_IFUNC symbol value.  */
	      h->root.u.def.value = sym->st_value;
	      h->root.u.def.section = sec;
	    }
	  defined_local = true;
	  resolved_local = true;
	  resolved_dynly = false;
	  resolved_to_const = false;
	  if (bfd_link_relocatable (info)
	      && ELF_ST_TYPE (sym->st_info) == STT_SECTION)
	    rel->r_addend += sec->output_offset;
	}
      else
	{
	  bool warned, ignored;

	  RELOC_FOR_GLOBAL_SYMBOL (info, input_bfd, input_section, rel,
				   r_symndx, symtab_hdr, sym_hashes,
				   h, sec, relocation,
				   unresolved_reloc, warned, ignored);
	  /* Here means symbol isn't local symbol only and 'h != NULL'.  */

	  /* The 'unresolved_syms_in_objects' specify how to deal with undefined
	     symbol.  And 'dynamic_undefined_weak' specify what to do when
	     meeting undefweak.  */

	  if ((is_undefweak = h->root.type == bfd_link_hash_undefweak))
	    {
	      defined_local = false;
	      resolved_local = false;
	      resolved_to_const = (!is_dyn || h->dynindx == -1
				   || UNDEFWEAK_NO_DYNAMIC_RELOC (info, h));
	      resolved_dynly = !resolved_local && !resolved_to_const;
	    }
	  else if (warned)
	    {
	      /* Symbol undefined offen means failed already.  I don't know why
		 'warned' here but I guess it want to continue relocating as if
		 no error occures to find other errors as more as possible.  */

	      /* To avoid generating warning messages about truncated
		 relocations, set the relocation's address to be the same as
		 the start of this section.  */
	      relocation = (input_section->output_section
			    ? input_section->output_section->vma
			    : 0);

	      defined_local = relocation != 0;
	      resolved_local = defined_local;
	      resolved_to_const = !resolved_local;
	      resolved_dynly = false;
	    }
	  else
	    {
	      defined_local = !unresolved_reloc && !ignored;
	      resolved_local =
		defined_local && SYMBOL_REFERENCES_LOCAL (info, h);
	      resolved_dynly = !resolved_local;
	      resolved_to_const = !resolved_local && !resolved_dynly;
	    }
	}

      name = loongarch_sym_name (input_bfd, h, sym);

      if (sec != NULL && discarded_section (sec))
	RELOC_AGAINST_DISCARDED_SECTION (info, input_bfd, input_section, rel,
					 1, relend, howto, 0, contents);

      if (bfd_link_relocatable (info))
	continue;

      /* The r_symndx will be STN_UNDEF (zero) only for relocs against symbols
	 from removed linkonce sections, or sections discarded by a linker
	 script.  Also for R_*_SOP_PUSH_ABSOLUTE and PCREL to specify const.  */
      if (r_symndx == STN_UNDEF || bfd_is_abs_section (sec))
	{
	  defined_local = false;
	  resolved_local = false;
	  resolved_dynly = false;
	  resolved_to_const = true;
	}

      /* The ifunc without reference does not generate plt.  */
      if (h && h->type == STT_GNU_IFUNC && h->plt.offset != MINUS_ONE)
	{
	  defined_local = true;
	  resolved_local = true;
	  resolved_dynly = false;
	  resolved_to_const = false;
	  relocation = sec_addr (plt) + h->plt.offset;
	}

      unresolved_reloc = resolved_dynly;

      BFD_ASSERT (resolved_local + resolved_dynly + resolved_to_const == 1);

      /* BFD_ASSERT (!resolved_dynly || (h && h->dynindx != -1));.  */

      BFD_ASSERT (!resolved_local || defined_local);

      is_ie = false;
      switch (r_type)
	{
	case R_LARCH_MARK_PCREL:
	case R_LARCH_MARK_LA:
	case R_LARCH_NONE:
	  r = bfd_reloc_continue;
	  unresolved_reloc = false;
	  break;

	case R_LARCH_32:
	case R_LARCH_64:
	  if (resolved_dynly || (is_pic && resolved_local))
	    {
	      Elf_Internal_Rela outrel;

	      /* When generating a shared object, these relocations are copied
		 into the output file to be resolved at run time.  */

	      outrel.r_offset = _bfd_elf_section_offset (output_bfd, info,
							 input_section,
							 rel->r_offset);

	      unresolved_reloc = (!((bfd_vma) -2 <= outrel.r_offset)
				  && (input_section->flags & SEC_ALLOC));

	      outrel.r_offset += sec_addr (input_section);

	      /* A pointer point to a local ifunc symbol.  */
	      if(h
		 && h->type == STT_GNU_IFUNC
		 && (h->dynindx == -1
		     || h->forced_local
		     || bfd_link_executable(info)))
		{
		  outrel.r_info = ELFNN_R_INFO (0, R_LARCH_IRELATIVE);
		  outrel.r_addend = (h->root.u.def.value
				     + h->root.u.def.section->output_section->vma
				     + h->root.u.def.section->output_offset);

		  /* Dynamic relocations are stored in
		     1. .rela.ifunc section in PIC object.
		     2. .rela.got section in dynamic executable.
		     3. .rela.iplt section in static executable.  */
		  if (bfd_link_pic (info))
		    sreloc = htab->elf.irelifunc;
		  else if (htab->elf.splt != NULL)
		    sreloc = htab->elf.srelgot;
		  else
		    sreloc = htab->elf.irelplt;
		}
	      else if (resolved_dynly)
		{
		  outrel.r_info = ELFNN_R_INFO (h->dynindx, r_type);
		  outrel.r_addend = rel->r_addend;
		}
	      else
		{
		  outrel.r_info = ELFNN_R_INFO (0, R_LARCH_RELATIVE);
		  outrel.r_addend = relocation + rel->r_addend;
		}

	      if (unresolved_reloc)
		loongarch_elf_append_rela (output_bfd, sreloc, &outrel);
	    }

	  relocation += rel->r_addend;
	  break;

	case R_LARCH_ADD8:
	case R_LARCH_ADD16:
	case R_LARCH_ADD24:
	case R_LARCH_ADD32:
	case R_LARCH_ADD64:
	case R_LARCH_SUB8:
	case R_LARCH_SUB16:
	case R_LARCH_SUB24:
	case R_LARCH_SUB32:
	case R_LARCH_SUB64:
	  if (resolved_dynly)
	    fatal = (loongarch_reloc_is_fatal
		     (info, input_bfd, input_section, rel, howto,
		      bfd_reloc_undefined, is_undefweak, name,
		      "Can't be resolved dynamically.  "
		      "If this procedure is hand-written assembly,\n"
		      "there must be something like '.dword sym1 - sym2' "
		      "to generate these relocs\n"
		      "and we can't get known link-time address of "
		      "these symbols."));
	  else
	    relocation += rel->r_addend;
	  break;

	case R_LARCH_TLS_DTPREL32:
	case R_LARCH_TLS_DTPREL64:
	  if (resolved_dynly)
	    {
	      Elf_Internal_Rela outrel;

	      outrel.r_offset = _bfd_elf_section_offset (output_bfd, info,
							 input_section,
							 rel->r_offset);
	      unresolved_reloc = (!((bfd_vma) -2 <= outrel.r_offset)
				  && (input_section->flags & SEC_ALLOC));
	      outrel.r_info = ELFNN_R_INFO (h->dynindx, r_type);
	      outrel.r_offset += sec_addr (input_section);
	      outrel.r_addend = rel->r_addend;
	      if (unresolved_reloc)
		loongarch_elf_append_rela (output_bfd, sreloc, &outrel);
	      break;
	    }

	  if (resolved_to_const)
	    fatal = loongarch_reloc_is_fatal (info, input_bfd, input_section,
					      rel, howto,
					      bfd_reloc_notsupported,
					      is_undefweak, name,
					      "Internal:");
	  if (resolved_local)
	    {
	      if (!elf_hash_table (info)->tls_sec)
		{
		fatal = loongarch_reloc_is_fatal (info, input_bfd,
			  input_section, rel, howto, bfd_reloc_notsupported,
			  is_undefweak, name, "TLS section not be created");
		}
	      else
		relocation -= elf_hash_table (info)->tls_sec->vma;
	    }
	  else
	    {
	    fatal = loongarch_reloc_is_fatal (info, input_bfd,
		      input_section, rel, howto, bfd_reloc_undefined,
		      is_undefweak, name,
		      "TLS LE just can be resolved local only.");
	    }

	  break;

	case R_LARCH_SOP_PUSH_TLS_TPREL:
	  if (resolved_local)
	    {
	      if (!elf_hash_table (info)->tls_sec)
		fatal = (loongarch_reloc_is_fatal
			 (info, input_bfd, input_section, rel, howto,
			  bfd_reloc_notsupported, is_undefweak, name,
			  "TLS section not be created"));
	      else
		relocation -= elf_hash_table (info)->tls_sec->vma;
	    }
	  else
	    fatal = (loongarch_reloc_is_fatal
		     (info, input_bfd, input_section, rel, howto,
		      bfd_reloc_undefined, is_undefweak, name,
		      "TLS LE just can be resolved local only."));
	  break;

	case R_LARCH_SOP_PUSH_ABSOLUTE:
	  if (is_undefweak)
	    {
	      if (resolved_dynly)
		fatal = (loongarch_reloc_is_fatal
			 (info, input_bfd, input_section, rel, howto,
			  bfd_reloc_dangerous, is_undefweak, name,
			  "Someone require us to resolve undefweak "
			  "symbol dynamically.  \n"
			  "But this reloc can't be done.  "
			  "I think I can't throw error "
			  "for this\n"
			  "so I resolved it to 0.  "
			  "I suggest to re-compile with '-fpic'."));

	      relocation = 0;
	      unresolved_reloc = false;
	      break;
	    }

	  if (resolved_to_const)
	    {
	      relocation += rel->r_addend;
	      break;
	    }

	  if (is_pic)
	    {
	      fatal = (loongarch_reloc_is_fatal
		       (info, input_bfd, input_section, rel, howto,
			bfd_reloc_notsupported, is_undefweak, name,
			"Under PIC we don't know load address.  Re-compile "
			"with '-fpic'?"));
	      break;
	    }

	  if (resolved_dynly)
	    {
	      if (!(plt && h && h->plt.offset != MINUS_ONE))
		{
		  fatal = (loongarch_reloc_is_fatal
			   (info, input_bfd, input_section, rel, howto,
			    bfd_reloc_undefined, is_undefweak, name,
			    "Can't be resolved dynamically.  Try to re-compile "
			    "with '-fpic'?"));
		  break;
		}

	      if (rel->r_addend != 0)
		{
		  fatal = (loongarch_reloc_is_fatal
			   (info, input_bfd, input_section, rel, howto,
			    bfd_reloc_notsupported, is_undefweak, name,
			    "Shouldn't be with r_addend."));
		  break;
		}

	      relocation = sec_addr (plt) + h->plt.offset;
	      unresolved_reloc = false;
	      break;
	    }

	  if (resolved_local)
	    {
	      relocation += rel->r_addend;
	      break;
	    }

	  break;

	case R_LARCH_SOP_PUSH_PCREL:
	case R_LARCH_SOP_PUSH_PLT_PCREL:
	  unresolved_reloc = false;

	  if (resolved_to_const)
	    {
	      relocation += rel->r_addend;
	      break;
	    }
	  else if (is_undefweak)
	    {
	      i = 0, j = 0;
	      relocation = 0;
	      if (resolved_dynly)
		{
		  if (h && h->plt.offset != MINUS_ONE)
		    i = 1, j = 2;
		  else
		    fatal = (loongarch_reloc_is_fatal
			     (info, input_bfd, input_section, rel, howto,
			      bfd_reloc_dangerous, is_undefweak, name,
			      "Undefweak need to be resolved dynamically, "
			      "but PLT stub doesn't represent."));
		}
	    }
	  else
	    {
	      if (!(defined_local || (h && h->plt.offset != MINUS_ONE)))
		{
		  fatal = (loongarch_reloc_is_fatal
			   (info, input_bfd, input_section, rel, howto,
			    bfd_reloc_undefined, is_undefweak, name,
			    "PLT stub does not represent and "
			    "symbol not defined."));
		  break;
		}

	      if (resolved_local)
		i = 0, j = 2;
	      else /* if (resolved_dynly) */
		{
		  if (!(h && h->plt.offset != MINUS_ONE))
		    fatal = (loongarch_reloc_is_fatal
			     (info, input_bfd, input_section, rel, howto,
			      bfd_reloc_dangerous, is_undefweak, name,
			      "Internal: PLT stub doesn't represent.  "
			      "Resolve it with pcrel"));
		  i = 1, j = 3;
		}
	    }

	  for (; i < j; i++)
	    {
	      if ((i & 1) == 0 && defined_local)
		{
		  relocation -= pc;
		  relocation += rel->r_addend;
		  break;
		}

	      if ((i & 1) && h && h->plt.offset != MINUS_ONE)
		{
		  if (rel->r_addend != 0)
		    {
		      fatal = (loongarch_reloc_is_fatal
			       (info, input_bfd, input_section, rel, howto,
				bfd_reloc_notsupported, is_undefweak, name,
				"PLT shouldn't be with r_addend."));
		      break;
		    }
		  relocation = sec_addr (plt) + h->plt.offset - pc;
		  break;
		}
	    }
	  break;

	case R_LARCH_SOP_PUSH_GPREL:
	  unresolved_reloc = false;

	  if (rel->r_addend != 0)
	    {
	      fatal = (loongarch_reloc_is_fatal
		       (info, input_bfd, input_section, rel, howto,
			bfd_reloc_notsupported, is_undefweak, name,
			"Shouldn't be with r_addend."));
	      break;
	    }

	  if (h != NULL)
	    {
	      off = h->got.offset;

	      if (off == MINUS_ONE
		  && h->type != STT_GNU_IFUNC)
		{
		  fatal = (loongarch_reloc_is_fatal
			   (info, input_bfd, input_section, rel, howto,
			    bfd_reloc_notsupported, is_undefweak, name,
			    "Internal: GOT entry doesn't represent."));
		  break;
		}

	      /* Hidden symbol not has .got entry, only .got.plt entry
		 so gprel is (plt - got).  */
	      if (off == MINUS_ONE
		  && h->type == STT_GNU_IFUNC)
		{
		  if (h->plt.offset == (bfd_vma) -1)
		    {
		      abort();
		    }

		  bfd_vma plt_index = h->plt.offset / PLT_ENTRY_SIZE;
		  off = plt_index * GOT_ENTRY_SIZE;

		  if (htab->elf.splt != NULL)
		    {
		      /* Section .plt header is 2 times of plt entry.  */
		      off = sec_addr(htab->elf.sgotplt) + off
			- sec_addr(htab->elf.sgot);
		    }
		  else
		    {
		      /* Section iplt not has plt header.  */
		      off = sec_addr(htab->elf.igotplt) + off
			- sec_addr(htab->elf.sgot);
		    }
		}

	      if (!WILL_CALL_FINISH_DYNAMIC_SYMBOL (is_dyn, is_pic, h)
		  || (is_pic && SYMBOL_REFERENCES_LOCAL (info, h)))
		{
		  /* This is actually a static link, or it is a
		     -Bsymbolic link and the symbol is defined
		     locally, or the symbol was forced to be local
		     because of a version file.  We must initialize
		     this entry in the global offset table.  Since the
		     offset must always be a multiple of the word size,
		     we use the least significant bit to record whether
		     we have initialized it already.

		     When doing a dynamic link, we create a .rela.got
		     relocation entry to initialize the value.  This
		     is done in the finish_dynamic_symbol routine.  */

		  if (resolved_dynly)
		    {
		      fatal = (loongarch_reloc_is_fatal
			       (info, input_bfd, input_section, rel, howto,
				bfd_reloc_dangerous, is_undefweak, name,
				"Internal: here shouldn't dynamic."));
		    }

		  if (!(defined_local || resolved_to_const))
		    {
		      fatal = (loongarch_reloc_is_fatal
			       (info, input_bfd, input_section, rel, howto,
				bfd_reloc_undefined, is_undefweak, name,
				"Internal: "));
		      break;
		    }

		  if ((off & 1) != 0)
		    off &= ~1;
		  else
		    {
		      /* The pr21964-4. Create relocate entry.  */
		      if (is_pic && h->start_stop)
			{
			  asection *s;
			  Elf_Internal_Rela outrel;
			  /* We need to generate a R_LARCH_RELATIVE reloc
			     for the dynamic linker.  */
			  s = htab->elf.srelgot;
			  if (!s)
			    {
			      fatal = loongarch_reloc_is_fatal (info, input_bfd,
				    input_section, rel, howto,
				    bfd_reloc_notsupported, is_undefweak, name,
				    "Internal: '.rel.got' not represent");
			      break;
			    }

			  outrel.r_offset = sec_addr (got) + off;
			  outrel.r_info = ELFNN_R_INFO (0, R_LARCH_RELATIVE);
			  outrel.r_addend = relocation; /* Link-time addr.  */
			  loongarch_elf_append_rela (output_bfd, s, &outrel);
			}
		      bfd_put_NN (output_bfd, relocation, got->contents + off);
		      h->got.offset |= 1;
		    }
		}
	    }
	  else
	    {
	      if (!local_got_offsets)
		{
		  fatal = (loongarch_reloc_is_fatal
			   (info, input_bfd, input_section, rel, howto,
			    bfd_reloc_notsupported, is_undefweak, name,
			    "Internal: local got offsets not reporesent."));
		  break;
		}

	      off = local_got_offsets[r_symndx];

	      if (off == MINUS_ONE)
		{
		  fatal = (loongarch_reloc_is_fatal
			   (info, input_bfd, input_section, rel, howto,
			    bfd_reloc_notsupported, is_undefweak, name,
			    "Internal: GOT entry doesn't represent."));
		  break;
		}

	      /* The offset must always be a multiple of the word size.
		 So, we can use the least significant bit to record
		 whether we have already processed this entry.  */
	      if ((off & 1) != 0)
		off &= ~1;
	      else
		{
		  if (is_pic)
		    {
		      asection *s;
		      Elf_Internal_Rela outrel;
		      /* We need to generate a R_LARCH_RELATIVE reloc
			 for the dynamic linker.  */
		      s = htab->elf.srelgot;
		      if (!s)
			{
			  fatal = (loongarch_reloc_is_fatal
				   (info, input_bfd, input_section, rel, howto,
				    bfd_reloc_notsupported, is_undefweak, name,
				    "Internal: '.rel.got' not represent"));
			  break;
			}

		      outrel.r_offset = sec_addr (got) + off;
		      outrel.r_info = ELFNN_R_INFO (0, R_LARCH_RELATIVE);
		      outrel.r_addend = relocation; /* Link-time addr.  */
		      loongarch_elf_append_rela (output_bfd, s, &outrel);
		    }

		  bfd_put_NN (output_bfd, relocation, got->contents + off);
		  local_got_offsets[r_symndx] |= 1;
		}
	    }
	  relocation = off;
	  break;

	case R_LARCH_SOP_PUSH_TLS_GOT:
	case R_LARCH_SOP_PUSH_TLS_GD:
	  if (r_type == R_LARCH_SOP_PUSH_TLS_GOT)
	    is_ie = true;
	  unresolved_reloc = false;

	  if (rel->r_addend != 0)
	    {
	      fatal = (loongarch_reloc_is_fatal
		       (info, input_bfd, input_section, rel, howto,
			bfd_reloc_notsupported, is_undefweak, name,
			"Shouldn't be with r_addend."));
	      break;
	    }


	  if (resolved_to_const && is_undefweak && h->dynindx != -1)
	    {
	      /* What if undefweak? Let rtld make a decision.  */
	      resolved_to_const = resolved_local = false;
	      resolved_dynly = true;
	    }

	  if (resolved_to_const)
	    {
	      fatal = (loongarch_reloc_is_fatal
		       (info, input_bfd, input_section, rel, howto,
			bfd_reloc_notsupported, is_undefweak, name,
			"Internal: Shouldn't be resolved to const."));
	      break;
	    }

	  if (h != NULL)
	    {
	      off = h->got.offset;
	      h->got.offset |= 1;
	    }
	  else
	    {
	      off = local_got_offsets[r_symndx];
	      local_got_offsets[r_symndx] |= 1;
	    }

	  if (off == MINUS_ONE)
	    {
	      fatal = (loongarch_reloc_is_fatal
		       (info, input_bfd, input_section, rel, howto,
			bfd_reloc_notsupported, is_undefweak, name,
			"Internal: TLS GOT entry doesn't represent."));
	      break;
	    }

	  tls_type = _bfd_loongarch_elf_tls_type (input_bfd, h, r_symndx);

	  /* If this symbol is referenced by both GD and IE TLS, the IE
	     reference's GOT slot follows the GD reference's slots.  */
	  ie_off = 0;
	  if ((tls_type & GOT_TLS_GD) && (tls_type & GOT_TLS_IE))
	    ie_off = 2 * GOT_ENTRY_SIZE;

	  if ((off & 1) != 0)
	    off &= ~1;
	  else
	    {
	      bfd_vma tls_block_off = 0;
	      Elf_Internal_Rela outrel;

	      if (resolved_local)
		{
		  if (!elf_hash_table (info)->tls_sec)
		    {
		      fatal = (loongarch_reloc_is_fatal
			       (info, input_bfd, input_section, rel, howto,
				bfd_reloc_notsupported, is_undefweak, name,
				"Internal: TLS sec not represent."));
		      break;
		    }
		  tls_block_off =
		    relocation - elf_hash_table (info)->tls_sec->vma;
		}

	      if (tls_type & GOT_TLS_GD)
		{
		  outrel.r_offset = sec_addr (got) + off;
		  outrel.r_addend = 0;
		  bfd_put_NN (output_bfd, 0, got->contents + off);
		  if (resolved_local && bfd_link_executable (info))
		    bfd_put_NN (output_bfd, 1, got->contents + off);
		  else if (resolved_local /* && !bfd_link_executable (info) */)
		    {
		      outrel.r_info = ELFNN_R_INFO (0, R_LARCH_TLS_DTPMODNN);
		      loongarch_elf_append_rela (output_bfd, htab->elf.srelgot,
						 &outrel);
		    }
		  else /* if (resolved_dynly) */
		    {
		      outrel.r_info =
			ELFNN_R_INFO (h->dynindx, R_LARCH_TLS_DTPMODNN);
		      loongarch_elf_append_rela (output_bfd, htab->elf.srelgot,
						 &outrel);
		    }

		  outrel.r_offset += GOT_ENTRY_SIZE;
		  bfd_put_NN (output_bfd, tls_block_off,
			      got->contents + off + GOT_ENTRY_SIZE);
		  if (resolved_local)
		    /* DTPREL known.  */;
		  else /* if (resolved_dynly) */
		    {
		      outrel.r_info =
			ELFNN_R_INFO (h->dynindx, R_LARCH_TLS_DTPRELNN);
		      loongarch_elf_append_rela (output_bfd, htab->elf.srelgot,
						 &outrel);
		    }
		}

	      if (tls_type & GOT_TLS_IE)
		{
		  outrel.r_offset = sec_addr (got) + off + ie_off;
		  bfd_put_NN (output_bfd, tls_block_off,
			      got->contents + off + ie_off);
		  if (resolved_local && bfd_link_executable (info))
		    /* TPREL known.  */;
		  else if (resolved_local /* && !bfd_link_executable (info) */)
		    {
		      outrel.r_info = ELFNN_R_INFO (0, R_LARCH_TLS_TPRELNN);
		      outrel.r_addend = tls_block_off;
		      loongarch_elf_append_rela (output_bfd, htab->elf.srelgot,
						 &outrel);
		    }
		  else /* if (resolved_dynly) */
		    {
		      /* Static linking has no .dynsym table.  */
		      if (!htab->elf.dynamic_sections_created)
			{
			  outrel.r_info =
			    ELFNN_R_INFO (0, R_LARCH_TLS_TPRELNN);
			  outrel.r_addend = 0;
			}
		      else
			{
			  outrel.r_info =
			    ELFNN_R_INFO (h->dynindx, R_LARCH_TLS_TPRELNN);
			  outrel.r_addend = 0;
			}
		      loongarch_elf_append_rela (output_bfd, htab->elf.srelgot,
						 &outrel);
		    }
		}
	    }

	  relocation = off + (is_ie ? ie_off : 0);
	  break;

	default:
	  break;
	}

      if (fatal)
	break;

      do
	{
	  /* 'unresolved_reloc' means we haven't done it yet.
	     We need help of dynamic linker to fix this memory location up.  */
	  if (!unresolved_reloc)
	    break;

	  if (_bfd_elf_section_offset (output_bfd, info, input_section,
				       rel->r_offset) == MINUS_ONE)
	    /* WHY? May because it's invalid so skip checking.
	       But why dynamic reloc a invalid section? */
	    break;

	  if (input_section->output_section->flags & SEC_DEBUGGING)
	    {
	      fatal = (loongarch_reloc_is_fatal
		       (info, input_bfd, input_section, rel, howto,
			bfd_reloc_dangerous, is_undefweak, name,
			"Seems dynamic linker not process "
			"sections 'SEC_DEBUGGING'."));
	    }
	  if (!is_dyn)
	    break;

	  if ((info->flags & DF_TEXTREL) == 0)
	    if (input_section->output_section->flags & SEC_READONLY)
	      info->flags |= DF_TEXTREL;
	}
      while (0);

      if (fatal)
	break;

      loongarch_record_one_reloc (input_bfd, input_section, r_type,
				  rel->r_offset, sym, h, rel->r_addend);

      if (r != bfd_reloc_continue)
	r = perform_relocation (rel, input_section, howto, relocation,
				input_bfd, contents);

      switch (r)
	{
	case bfd_reloc_dangerous:
	case bfd_reloc_continue:
	case bfd_reloc_ok:
	  continue;

	case bfd_reloc_overflow:
	  /* Overflow value can't be filled in.  */
	  loongarch_dump_reloc_record (info->callbacks->info);
	  info->callbacks->reloc_overflow
	    (info, h ? &h->root : NULL, name, howto->name, rel->r_addend,
	     input_bfd, input_section, rel->r_offset);
	  break;

	case bfd_reloc_outofrange:
	  /* Stack state incorrect.  */
	  loongarch_dump_reloc_record (info->callbacks->info);
	  info->callbacks->info
	    ("%X%H: Internal stack state is incorrect.\n"
	     "Want to push to full stack or pop from empty stack?\n",
	     input_bfd, input_section, rel->r_offset);
	  break;

	case bfd_reloc_notsupported:
	  info->callbacks->info ("%X%H: Unknown relocation type.\n", input_bfd,
				 input_section, rel->r_offset);
	  break;

	default:
	  info->callbacks->info ("%X%H: Internal: unknown error.\n", input_bfd,
				 input_section, rel->r_offset);
	  break;
	}

      fatal = true;
      break;
    }

  return !fatal;
}

/* Finish up dynamic symbol handling.  We set the contents of various
   dynamic sections here.  */

static bool
loongarch_elf_finish_dynamic_symbol (bfd *output_bfd,
				     struct bfd_link_info *info,
				     struct elf_link_hash_entry *h,
				     Elf_Internal_Sym *sym)
{
  struct loongarch_elf_link_hash_table *htab = loongarch_elf_hash_table (info);
  const struct elf_backend_data *bed = get_elf_backend_data (output_bfd);

  if (h->plt.offset != MINUS_ONE)
    {
      size_t i, plt_idx;
      asection *plt, *gotplt, *relplt;
      bfd_vma got_address;
      uint32_t plt_entry[PLT_ENTRY_INSNS];
      bfd_byte *loc;
      Elf_Internal_Rela rela;

      if (htab->elf.splt)
	{
	  BFD_ASSERT ((h->type == STT_GNU_IFUNC
		       && SYMBOL_REFERENCES_LOCAL (info, h))
		      || h->dynindx != -1);

	  plt = htab->elf.splt;
	  gotplt = htab->elf.sgotplt;
	  relplt = htab->elf.srelplt;
	  plt_idx = (h->plt.offset - PLT_HEADER_SIZE) / PLT_ENTRY_SIZE;
	  got_address =
	    sec_addr (gotplt) + GOTPLT_HEADER_SIZE + plt_idx * GOT_ENTRY_SIZE;
	}
      else /* if (htab->elf.iplt) */
	{
	  BFD_ASSERT (h->type == STT_GNU_IFUNC
		      && SYMBOL_REFERENCES_LOCAL (info, h));

	  plt = htab->elf.iplt;
	  gotplt = htab->elf.igotplt;
	  relplt = htab->elf.irelplt;
	  plt_idx = h->plt.offset / PLT_ENTRY_SIZE;
	  got_address = sec_addr (gotplt) + plt_idx * GOT_ENTRY_SIZE;
	}

      /* Find out where the .plt entry should go.  */
      loc = plt->contents + h->plt.offset;

      /* Fill in the PLT entry itself.  */
      if (!loongarch_make_plt_entry (got_address,
				     sec_addr (plt) + h->plt.offset,
				     plt_entry))
	return false;

      for (i = 0; i < PLT_ENTRY_INSNS; i++)
	bfd_put_32 (output_bfd, plt_entry[i], loc + 4 * i);

      /* Fill in the initial value of the .got.plt entry.  */
      loc = gotplt->contents + (got_address - sec_addr (gotplt));
      bfd_put_NN (output_bfd, sec_addr (plt), loc);

      rela.r_offset = got_address;

      /* TRUE if this is a PLT reference to a local IFUNC.  */
      if (PLT_LOCAL_IFUNC_P(info, h))
	{
	  rela.r_info = ELFNN_R_INFO (0, R_LARCH_IRELATIVE);
	  rela.r_addend = (h->root.u.def.value
			   + h->root.u.def.section->output_section->vma
			   + h->root.u.def.section->output_offset);
	}
      else
	{
	  /* Fill in the entry in the .rela.plt section.  */
	  rela.r_info = ELFNN_R_INFO (h->dynindx, R_LARCH_JUMP_SLOT);
	  rela.r_addend = 0;
	}

      loc = relplt->contents + plt_idx * sizeof (ElfNN_External_Rela);
      bed->s->swap_reloca_out (output_bfd, &rela, loc);

      if (!h->def_regular)
	{
	  /* Mark the symbol as undefined, rather than as defined in
	     the .plt section.  Leave the value alone.  */
	  sym->st_shndx = SHN_UNDEF;
	  /* If the symbol is weak, we do need to clear the value.
	     Otherwise, the PLT entry would provide a definition for
	     the symbol even if the symbol wasn't defined anywhere,
	     and so the symbol would never be NULL.  */
	  if (!h->ref_regular_nonweak)
	    sym->st_value = 0;
	}
    }

  if (h->got.offset != MINUS_ONE
      /* TLS got entry have been handled in elf_relocate_section.  */
      && !(loongarch_elf_hash_entry (h)->tls_type & (GOT_TLS_GD | GOT_TLS_IE))
      /* have allocated got entry but not allocated rela before.  */
      && !UNDEFWEAK_NO_DYNAMIC_RELOC (info, h))
    {
      asection *sgot, *srela;
      Elf_Internal_Rela rela;
      bfd_vma off = h->got.offset & ~(bfd_vma) 1;

      /* This symbol has an entry in the GOT.  Set it up.  */

      sgot = htab->elf.sgot;
      srela = htab->elf.srelgot;
      BFD_ASSERT (sgot && srela);

      rela.r_offset = sec_addr (sgot) + off;

      if (h->def_regular
	  && h->type == STT_GNU_IFUNC)
	{
	  if(h->plt.offset == MINUS_ONE)
	    {
	      if (htab->elf.splt == NULL)
		srela = htab->elf.irelplt;

	      if (SYMBOL_REFERENCES_LOCAL (info, h))
		{
		  asection *sec = h->root.u.def.section;
		  rela.r_info = ELFNN_R_INFO (0, R_LARCH_IRELATIVE);
		  rela.r_addend = h->root.u.def.value + sec->output_section->vma
		    + sec->output_offset;
		  bfd_put_NN (output_bfd, 0, sgot->contents + off);
		}
	      else
		{
		  BFD_ASSERT ((h->got.offset & 1) == 0);
		  BFD_ASSERT (h->dynindx != -1);
		  rela.r_info = ELFNN_R_INFO (h->dynindx, R_LARCH_NN);
		  rela.r_addend = 0;
		  bfd_put_NN (output_bfd, (bfd_vma) 0, sgot->contents + off);
		}
	    }
	  else if(bfd_link_pic (info))
	    {
	      rela.r_info = ELFNN_R_INFO (h->dynindx, R_LARCH_NN);
	      rela.r_addend = 0;
	      bfd_put_NN (output_bfd, rela.r_addend, sgot->contents + off);
	    }
	  else
	    {
	      asection *plt;
	      /* For non-shared object, we can't use .got.plt, which
		 contains the real function address if we need pointer
		 equality.  We load the GOT entry with the PLT entry.  */
	      plt = htab->elf.splt ? htab->elf.splt : htab->elf.iplt;
	      bfd_put_NN (output_bfd,
			  (plt->output_section->vma
			   + plt->output_offset
			   + h->plt.offset),
			  sgot->contents + off);
	      return true;
	    }
	}
      else if (bfd_link_pic (info) && SYMBOL_REFERENCES_LOCAL (info, h))
	{
	  BFD_ASSERT (h->got.offset & 1 /* Has been filled in addr.  */);
	  asection *sec = h->root.u.def.section;
	  rela.r_info = ELFNN_R_INFO (0, R_LARCH_RELATIVE);
	  rela.r_addend = (h->root.u.def.value + sec->output_section->vma
			   + sec->output_offset);
	}
      else
	{
	  BFD_ASSERT ((h->got.offset & 1) == 0);
	  BFD_ASSERT (h->dynindx != -1);
	  rela.r_info = ELFNN_R_INFO (h->dynindx, R_LARCH_NN);
	  rela.r_addend = 0;
	}

      loongarch_elf_append_rela (output_bfd, srela, &rela);
    }

  if (h->needs_copy)
    {
      Elf_Internal_Rela rela;
      asection *s;

      /* This symbols needs a copy reloc.  Set it up.  */
      BFD_ASSERT (h->dynindx != -1);

      rela.r_offset = sec_addr (h->root.u.def.section) + h->root.u.def.value;
      rela.r_info = ELFNN_R_INFO (h->dynindx, R_LARCH_COPY);
      rela.r_addend = 0;
      if (h->root.u.def.section == htab->elf.sdynrelro)
	s = htab->elf.sreldynrelro;
      else
	s = htab->elf.srelbss;
      loongarch_elf_append_rela (output_bfd, s, &rela);
    }

  /* Mark some specially defined symbols as absolute.  */
  if (h == htab->elf.hdynamic || h == htab->elf.hgot || h == htab->elf.hplt)
    sym->st_shndx = SHN_ABS;

  return true;
}

/* Finish up the dynamic sections.  */

static bool
loongarch_finish_dyn (bfd *output_bfd, struct bfd_link_info *info, bfd *dynobj,
		      asection *sdyn)
{
  struct loongarch_elf_link_hash_table *htab = loongarch_elf_hash_table (info);
  const struct elf_backend_data *bed = get_elf_backend_data (output_bfd);
  size_t dynsize = bed->s->sizeof_dyn, skipped_size = 0;
  bfd_byte *dyncon, *dynconend;

  dynconend = sdyn->contents + sdyn->size;
  for (dyncon = sdyn->contents; dyncon < dynconend; dyncon += dynsize)
    {
      Elf_Internal_Dyn dyn;
      asection *s;
      int skipped = 0;

      bed->s->swap_dyn_in (dynobj, dyncon, &dyn);

      switch (dyn.d_tag)
	{
	case DT_PLTGOT:
	  s = htab->elf.sgotplt;
	  dyn.d_un.d_ptr = s->output_section->vma + s->output_offset;
	  break;
	case DT_JMPREL:
	  s = htab->elf.srelplt;
	  dyn.d_un.d_ptr = s->output_section->vma + s->output_offset;
	  break;
	case DT_PLTRELSZ:
	  s = htab->elf.srelplt;
	  dyn.d_un.d_val = s->size;
	  break;
	case DT_TEXTREL:
	  if ((info->flags & DF_TEXTREL) == 0)
	    skipped = 1;
	  break;
	case DT_FLAGS:
	  if ((info->flags & DF_TEXTREL) == 0)
	    dyn.d_un.d_val &= ~DF_TEXTREL;
	  break;
	}
      if (skipped)
	skipped_size += dynsize;
      else
	bed->s->swap_dyn_out (output_bfd, &dyn, dyncon - skipped_size);
    }
  /* Wipe out any trailing entries if we shifted down a dynamic tag.  */
  memset (dyncon - skipped_size, 0, skipped_size);
  return true;
}

/* Finish up local dynamic symbol handling.  We set the contents of
   various dynamic sections here.  */

static bool
elfNN_loongarch_finish_local_dynamic_symbol (void **slot, void *inf)
{
  struct elf_link_hash_entry *h = (struct elf_link_hash_entry *) *slot;
  struct bfd_link_info *info = (struct bfd_link_info *) inf;

  return loongarch_elf_finish_dynamic_symbol (info->output_bfd, info, h, NULL);
}

static bool
loongarch_elf_finish_dynamic_sections (bfd *output_bfd,
				       struct bfd_link_info *info)
{
  bfd *dynobj;
  asection *sdyn, *plt, *gotplt = NULL;
  struct loongarch_elf_link_hash_table *htab;

  htab = loongarch_elf_hash_table (info);
  BFD_ASSERT (htab);
  dynobj = htab->elf.dynobj;
  sdyn = bfd_get_linker_section (dynobj, ".dynamic");

  if (elf_hash_table (info)->dynamic_sections_created)
    {
      BFD_ASSERT (htab->elf.splt && sdyn);

      if (!loongarch_finish_dyn (output_bfd, info, dynobj, sdyn))
	return false;
    }

  plt = htab->elf.splt;
  gotplt = htab->elf.sgotplt;

  if (plt && 0 < plt->size)
    {
      size_t i;
      uint32_t plt_header[PLT_HEADER_INSNS];
      if (!loongarch_make_plt_header (sec_addr (gotplt), sec_addr (plt),
				      plt_header))
	return false;

      for (i = 0; i < PLT_HEADER_INSNS; i++)
	bfd_put_32 (output_bfd, plt_header[i], plt->contents + 4 * i);

      elf_section_data (plt->output_section)->this_hdr.sh_entsize =
	PLT_ENTRY_SIZE;
    }

  if (htab->elf.sgotplt)
    {
      asection *output_section = htab->elf.sgotplt->output_section;

      if (bfd_is_abs_section (output_section))
	{
	  _bfd_error_handler (_("discarded output section: `%pA'"),
			      htab->elf.sgotplt);
	  return false;
	}

      if (0 < htab->elf.sgotplt->size)
	{
	  /* Write the first two entries in .got.plt, needed for the dynamic
	     linker.  */
	  bfd_put_NN (output_bfd, MINUS_ONE, htab->elf.sgotplt->contents);

	  bfd_put_NN (output_bfd, (bfd_vma) 0,
		      htab->elf.sgotplt->contents + GOT_ENTRY_SIZE);
	}

      elf_section_data (output_section)->this_hdr.sh_entsize = GOT_ENTRY_SIZE;
    }

  if (htab->elf.sgot)
    {
      asection *output_section = htab->elf.sgot->output_section;

      if (0 < htab->elf.sgot->size)
	{
	  /* Set the first entry in the global offset table to the address of
	     the dynamic section.  */
	  bfd_vma val = sdyn ? sec_addr (sdyn) : 0;
	  bfd_put_NN (output_bfd, val, htab->elf.sgot->contents);
	}

      elf_section_data (output_section)->this_hdr.sh_entsize = GOT_ENTRY_SIZE;
    }

  /* Fill PLT and GOT entries for local STT_GNU_IFUNC symbols.  */
  htab_traverse (htab->loc_hash_table,
		 (void *) elfNN_loongarch_finish_local_dynamic_symbol, info);

  return true;
}

/* Return address for Ith PLT stub in section PLT, for relocation REL
   or (bfd_vma) -1 if it should not be included.  */

static bfd_vma
loongarch_elf_plt_sym_val (bfd_vma i, const asection *plt,
			   const arelent *rel ATTRIBUTE_UNUSED)
{
  return plt->vma + PLT_HEADER_SIZE + i * PLT_ENTRY_SIZE;
}

static enum elf_reloc_type_class
loongarch_reloc_type_class (const struct bfd_link_info *info ATTRIBUTE_UNUSED,
			    const asection *rel_sec ATTRIBUTE_UNUSED,
			    const Elf_Internal_Rela *rela)
{
  struct loongarch_elf_link_hash_table *htab;
  htab = loongarch_elf_hash_table (info);

  if (htab->elf.dynsym != NULL && htab->elf.dynsym->contents != NULL)
    {
      /* Check relocation against STT_GNU_IFUNC symbol if there are
	 dynamic symbols.  */
      bfd *abfd = info->output_bfd;
      const struct elf_backend_data *bed = get_elf_backend_data (abfd);
      unsigned long r_symndx = ELFNN_R_SYM (rela->r_info);
      if (r_symndx != STN_UNDEF)
	{
	  Elf_Internal_Sym sym;
	  if (!bed->s->swap_symbol_in (abfd,
				       htab->elf.dynsym->contents
				       + r_symndx * bed->s->sizeof_sym,
				       0, &sym))
	    {
	      /* xgettext:c-format  */
	      _bfd_error_handler (_("%pB symbol number %lu references"
				    " nonexistent SHT_SYMTAB_SHNDX section"),
				  abfd, r_symndx);
	      /* Ideally an error class should be returned here.  */
	    }
	  else if (ELF_ST_TYPE (sym.st_info) == STT_GNU_IFUNC)
	    return reloc_class_ifunc;
	}
    }

  switch (ELFNN_R_TYPE (rela->r_info))
    {
    case R_LARCH_IRELATIVE:
      return reloc_class_ifunc;
    case R_LARCH_RELATIVE:
      return reloc_class_relative;
    case R_LARCH_JUMP_SLOT:
      return reloc_class_plt;
    case R_LARCH_COPY:
      return reloc_class_copy;
    default:
      return reloc_class_normal;
    }
}

/* Copy the extra info we tack onto an elf_link_hash_entry.  */

static void
loongarch_elf_copy_indirect_symbol (struct bfd_link_info *info,
				    struct elf_link_hash_entry *dir,
				    struct elf_link_hash_entry *ind)
{
  struct elf_link_hash_entry *edir, *eind;

  edir = dir;
  eind = ind;

  if (eind->dyn_relocs != NULL)
    {
      if (edir->dyn_relocs != NULL)
	{
	  struct elf_dyn_relocs **pp;
	  struct elf_dyn_relocs *p;

	  /* Add reloc counts against the indirect sym to the direct sym
	     list.  Merge any entries against the same section.  */
	  for (pp = &eind->dyn_relocs; (p = *pp) != NULL;)
	    {
	      struct elf_dyn_relocs *q;

	      for (q = edir->dyn_relocs; q != NULL; q = q->next)
		if (q->sec == p->sec)
		  {
		    q->pc_count += p->pc_count;
		    q->count += p->count;
		    *pp = p->next;
		    break;
		  }
	      if (q == NULL)
		pp = &p->next;
	    }
	  *pp = edir->dyn_relocs;
	}

      edir->dyn_relocs = eind->dyn_relocs;
      eind->dyn_relocs = NULL;
    }

  if (ind->root.type == bfd_link_hash_indirect && dir->got.refcount < 0)
    {
      loongarch_elf_hash_entry(edir)->tls_type
	= loongarch_elf_hash_entry(eind)->tls_type;
      loongarch_elf_hash_entry(eind)->tls_type = GOT_UNKNOWN;
    }
  _bfd_elf_link_hash_copy_indirect (info, dir, ind);
}

#define PRSTATUS_SIZE		    0x1d8
#define PRSTATUS_OFFSET_PR_CURSIG   0xc
#define PRSTATUS_OFFSET_PR_PID	    0x20
#define ELF_GREGSET_T_SIZE	    0x168
#define PRSTATUS_OFFSET_PR_REG	    0x70

/* Support for core dump NOTE sections.  */

static bool
loongarch_elf_grok_prstatus (bfd *abfd, Elf_Internal_Note *note)
{
  switch (note->descsz)
    {
    default:
      return false;

    /* The sizeof (struct elf_prstatus) on Linux/LoongArch.  */
    case PRSTATUS_SIZE:
      /* pr_cursig  */
      elf_tdata (abfd)->core->signal =
	bfd_get_16 (abfd, note->descdata + PRSTATUS_OFFSET_PR_CURSIG);

      /* pr_pid  */
      elf_tdata (abfd)->core->lwpid =
	bfd_get_32 (abfd, note->descdata + PRSTATUS_OFFSET_PR_PID);
      break;
    }

  /* Make a ".reg/999" section.  */
  return _bfd_elfcore_make_pseudosection (abfd, ".reg", ELF_GREGSET_T_SIZE,
					  note->descpos
					  + PRSTATUS_OFFSET_PR_REG);
}

#define PRPSINFO_SIZE		    0x88
#define PRPSINFO_OFFSET_PR_PID	    0x18
#define PRPSINFO_OFFSET_PR_FNAME    0x28
#define PRPSINFO_SIZEOF_PR_FNAME    0x10
#define PRPSINFO_OFFSET_PR_PS_ARGS  0x38
#define PRPSINFO_SIZEOF_PR_PS_ARGS  0x50


static bool
loongarch_elf_grok_psinfo (bfd *abfd, Elf_Internal_Note *note)
{
  switch (note->descsz)
    {
    default:
      return false;

    /* The sizeof (prpsinfo_t) on Linux/LoongArch.  */
    case PRPSINFO_SIZE:
      /* pr_pid  */
      elf_tdata (abfd)->core->pid =
	bfd_get_32 (abfd, note->descdata + PRPSINFO_OFFSET_PR_PID);

      /* pr_fname  */
      elf_tdata (abfd)->core->program =
	_bfd_elfcore_strndup (abfd, note->descdata + PRPSINFO_OFFSET_PR_FNAME,
			      PRPSINFO_SIZEOF_PR_FNAME);

      /* pr_psargs  */
      elf_tdata (abfd)->core->command =
	_bfd_elfcore_strndup (abfd, note->descdata + PRPSINFO_OFFSET_PR_PS_ARGS,
			      PRPSINFO_SIZEOF_PR_PS_ARGS);
      break;
    }

  /* Note that for some reason, a spurious space is tacked
     onto the end of the args in some (at least one anyway)
     implementations, so strip it off if it exists.  */

  {
    char *command = elf_tdata (abfd)->core->command;
    int n = strlen (command);

    if (0 < n && command[n - 1] == ' ')
      command[n - 1] = '\0';
  }

  return true;
}

/* Set the right mach type.  */
static bool
loongarch_elf_object_p (bfd *abfd)
{
  /* There are only two mach types in LoongArch currently.  */
  if (strcmp (abfd->xvec->name, "elf64-loongarch") == 0)
    bfd_default_set_arch_mach (abfd, bfd_arch_loongarch, bfd_mach_loongarch64);
  else
    bfd_default_set_arch_mach (abfd, bfd_arch_loongarch, bfd_mach_loongarch32);
  return true;
}

static asection *
loongarch_elf_gc_mark_hook (asection *sec, struct bfd_link_info *info,
			    Elf_Internal_Rela *rel,
			    struct elf_link_hash_entry *h,
			    Elf_Internal_Sym *sym)
{
  if (h != NULL)
    switch (ELFNN_R_TYPE (rel->r_info))
      {
      case R_LARCH_GNU_VTINHERIT:
      case R_LARCH_GNU_VTENTRY:
	return NULL;
      }

  return _bfd_elf_gc_mark_hook (sec, info, rel, h, sym);
}

/* Return TRUE if symbol H should be hashed in the `.gnu.hash' section.  For
   executable PLT slots where the executable never takes the address of those
   functions, the function symbols are not added to the hash table.  */

static bool
elf_loongarch64_hash_symbol (struct elf_link_hash_entry *h)
{
  if (h->plt.offset != (bfd_vma) -1
      && !h->def_regular
      && !h->pointer_equality_needed)
    return false;

  return _bfd_elf_hash_symbol (h);
}

#define TARGET_LITTLE_SYM loongarch_elfNN_vec
#define TARGET_LITTLE_NAME "elfNN-loongarch"
#define ELF_ARCH bfd_arch_loongarch
#define ELF_TARGET_ID LARCH_ELF_DATA
#define ELF_MACHINE_CODE EM_LOONGARCH
#define ELF_MAXPAGESIZE 0x4000
#define bfd_elfNN_bfd_reloc_type_lookup loongarch_reloc_type_lookup
#define bfd_elfNN_bfd_link_hash_table_create				  \
  loongarch_elf_link_hash_table_create
#define bfd_elfNN_bfd_reloc_name_lookup loongarch_reloc_name_lookup
#define elf_info_to_howto_rel NULL /* Fall through to elf_info_to_howto.  */
#define elf_info_to_howto loongarch_info_to_howto_rela
#define bfd_elfNN_bfd_merge_private_bfd_data				  \
  elfNN_loongarch_merge_private_bfd_data

#define elf_backend_reloc_type_class loongarch_reloc_type_class
#define elf_backend_copy_indirect_symbol loongarch_elf_copy_indirect_symbol
#define elf_backend_create_dynamic_sections				   \
  loongarch_elf_create_dynamic_sections
#define elf_backend_check_relocs loongarch_elf_check_relocs
#define elf_backend_adjust_dynamic_symbol loongarch_elf_adjust_dynamic_symbol
#define elf_backend_size_dynamic_sections loongarch_elf_size_dynamic_sections
#define elf_backend_relocate_section loongarch_elf_relocate_section
#define elf_backend_finish_dynamic_symbol loongarch_elf_finish_dynamic_symbol
#define elf_backend_finish_dynamic_sections				   \
  loongarch_elf_finish_dynamic_sections
#define elf_backend_object_p loongarch_elf_object_p
#define elf_backend_gc_mark_hook loongarch_elf_gc_mark_hook
#define elf_backend_plt_sym_val loongarch_elf_plt_sym_val
#define elf_backend_grok_prstatus loongarch_elf_grok_prstatus
#define elf_backend_grok_psinfo loongarch_elf_grok_psinfo
#define elf_backend_hash_symbol elf_loongarch64_hash_symbol

#include "elfNN-target.h"
