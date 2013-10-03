/* Handle SVR4 shared libraries for GDB, the GNU Debugger.

   Copyright (C) 1990-2013 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "defs.h"

#include "elf/external.h"
#include "elf/common.h"
#include "elf/mips.h"

#include "symtab.h"
#include "bfd.h"
#include "symfile.h"
#include "objfiles.h"
#include "gdbcore.h"
#include "target.h"
#include "inferior.h"
#include "regcache.h"
#include "gdbthread.h"
#include "observer.h"

#include "gdb_assert.h"

#include "solist.h"
#include "solib.h"
#include "solib-svr4.h"

#include "bfd-target.h"
#include "elf-bfd.h"
#include "exec.h"
#include "auxv.h"
#include "exceptions.h"
#include "gdb_bfd.h"

static struct link_map_offsets *svr4_fetch_link_map_offsets (void);
static int svr4_have_link_map_offsets (void);
static void svr4_relocate_main_executable (void);

/* Link map info to include in an allocated so_list entry.  */

struct lm_info
  {
    /* Amount by which addresses in the binary should be relocated to
       match the inferior.  The direct inferior value is L_ADDR_INFERIOR.
       When prelinking is involved and the prelink base address changes,
       we may need a different offset - the recomputed offset is in L_ADDR.
       It is commonly the same value.  It is cached as we want to warn about
       the difference and compute it only once.  L_ADDR is valid
       iff L_ADDR_P.  */
    CORE_ADDR l_addr, l_addr_inferior;
    unsigned int l_addr_p : 1;

    /* The target location of lm.  */
    CORE_ADDR lm_addr;

    /* Values read in from inferior's fields of the same name.  */
    CORE_ADDR l_ld, l_next, l_prev, l_name;
  };

/* On SVR4 systems, a list of symbols in the dynamic linker where
   GDB can try to place a breakpoint to monitor shared library
   events.

   If none of these symbols are found, or other errors occur, then
   SVR4 systems will fall back to using a symbol as the "startup
   mapping complete" breakpoint address.  */

static const char * const solib_break_names[] =
{
  "r_debug_state",
  "_r_debug_state",
  "_dl_debug_state",
  "rtld_db_dlactivity",
  "__dl_rtld_db_dlactivity",
  "_rtld_debug_state",

  NULL
};

static const char * const bkpt_names[] =
{
  "_start",
  "__start",
  "main",
  NULL
};

static const  char * const main_name_list[] =
{
  "main_$main",
  NULL
};

/* Return non-zero if GDB_SO_NAME and INFERIOR_SO_NAME represent
   the same shared library.  */

static int
svr4_same_1 (const char *gdb_so_name, const char *inferior_so_name)
{
  if (strcmp (gdb_so_name, inferior_so_name) == 0)
    return 1;

  /* On Solaris, when starting inferior we think that dynamic linker is
     /usr/lib/ld.so.1, but later on, the table of loaded shared libraries
     contains /lib/ld.so.1.  Sometimes one file is a link to another, but
     sometimes they have identical content, but are not linked to each
     other.  We don't restrict this check for Solaris, but the chances
     of running into this situation elsewhere are very low.  */
  if (strcmp (gdb_so_name, "/usr/lib/ld.so.1") == 0
      && strcmp (inferior_so_name, "/lib/ld.so.1") == 0)
    return 1;

  /* Similarly, we observed the same issue with sparc64, but with
     different locations.  */
  if (strcmp (gdb_so_name, "/usr/lib/sparcv9/ld.so.1") == 0
      && strcmp (inferior_so_name, "/lib/sparcv9/ld.so.1") == 0)
    return 1;

  return 0;
}

static int
svr4_same (struct so_list *gdb, struct so_list *inferior)
{
  return (svr4_same_1 (gdb->so_original_name, inferior->so_original_name));
}

static struct lm_info *
lm_info_read (CORE_ADDR lm_addr)
{
  struct link_map_offsets *lmo = svr4_fetch_link_map_offsets ();
  gdb_byte *lm;
  struct lm_info *lm_info;
  struct cleanup *back_to;

  lm = xmalloc (lmo->link_map_size);
  back_to = make_cleanup (xfree, lm);

  if (target_read_memory (lm_addr, lm, lmo->link_map_size) != 0)
    {
      warning (_("Error reading shared library list entry at %s"),
	       paddress (target_gdbarch (), lm_addr)),
      lm_info = NULL;
    }
  else
    {
      struct type *ptr_type = builtin_type (target_gdbarch ())->builtin_data_ptr;

      lm_info = xzalloc (sizeof (*lm_info));
      lm_info->lm_addr = lm_addr;

      lm_info->l_addr_inferior = extract_typed_address (&lm[lmo->l_addr_offset],
							ptr_type);
      lm_info->l_ld = extract_typed_address (&lm[lmo->l_ld_offset], ptr_type);
      lm_info->l_next = extract_typed_address (&lm[lmo->l_next_offset],
					       ptr_type);
      lm_info->l_prev = extract_typed_address (&lm[lmo->l_prev_offset],
					       ptr_type);
      lm_info->l_name = extract_typed_address (&lm[lmo->l_name_offset],
					       ptr_type);
    }

  do_cleanups (back_to);

  return lm_info;
}

static int
has_lm_dynamic_from_link_map (void)
{
  struct link_map_offsets *lmo = svr4_fetch_link_map_offsets ();

  return lmo->l_ld_offset >= 0;
}

static CORE_ADDR
lm_addr_check (struct so_list *so, bfd *abfd)
{
  if (!so->lm_info->l_addr_p)
    {
      struct bfd_section *dyninfo_sect;
      CORE_ADDR l_addr, l_dynaddr, dynaddr;

      l_addr = so->lm_info->l_addr_inferior;

      if (! abfd || ! has_lm_dynamic_from_link_map ())
	goto set_addr;

      l_dynaddr = so->lm_info->l_ld;

      dyninfo_sect = bfd_get_section_by_name (abfd, ".dynamic");
      if (dyninfo_sect == NULL)
	goto set_addr;

      dynaddr = bfd_section_vma (abfd, dyninfo_sect);

      if (dynaddr + l_addr != l_dynaddr)
	{
	  CORE_ADDR align = 0x1000;
	  CORE_ADDR minpagesize = align;

	  if (bfd_get_flavour (abfd) == bfd_target_elf_flavour)
	    {
	      Elf_Internal_Ehdr *ehdr = elf_tdata (abfd)->elf_header;
	      Elf_Internal_Phdr *phdr = elf_tdata (abfd)->phdr;
	      int i;

	      align = 1;

	      for (i = 0; i < ehdr->e_phnum; i++)
		if (phdr[i].p_type == PT_LOAD && phdr[i].p_align > align)
		  align = phdr[i].p_align;

	      minpagesize = get_elf_backend_data (abfd)->minpagesize;
	    }

	  /* Turn it into a mask.  */
	  align--;

	  /* If the changes match the alignment requirements, we
	     assume we're using a core file that was generated by the
	     same binary, just prelinked with a different base offset.
	     If it doesn't match, we may have a different binary, the
	     same binary with the dynamic table loaded at an unrelated
	     location, or anything, really.  To avoid regressions,
	     don't adjust the base offset in the latter case, although
	     odds are that, if things really changed, debugging won't
	     quite work.

	     One could expect more the condition
	       ((l_addr & align) == 0 && ((l_dynaddr - dynaddr) & align) == 0)
	     but the one below is relaxed for PPC.  The PPC kernel supports
	     either 4k or 64k page sizes.  To be prepared for 64k pages,
	     PPC ELF files are built using an alignment requirement of 64k.
	     However, when running on a kernel supporting 4k pages, the memory
	     mapping of the library may not actually happen on a 64k boundary!

	     (In the usual case where (l_addr & align) == 0, this check is
	     equivalent to the possibly expected check above.)

	     Even on PPC it must be zero-aligned at least for MINPAGESIZE.  */

	  l_addr = l_dynaddr - dynaddr;

	  if ((l_addr & (minpagesize - 1)) == 0
	      && (l_addr & align) == ((l_dynaddr - dynaddr) & align))
	    {
	      if (info_verbose)
		printf_unfiltered (_("Using PIC (Position Independent Code) "
				     "prelink displacement %s for \"%s\".\n"),
				   paddress (target_gdbarch (), l_addr),
				   so->so_name);
	    }
	  else
	    {
	      /* There is no way to verify the library file matches.  prelink
		 can during prelinking of an unprelinked file (or unprelinking
		 of a prelinked file) shift the DYNAMIC segment by arbitrary
		 offset without any page size alignment.  There is no way to
		 find out the ELF header and/or Program Headers for a limited
		 verification if it they match.  One could do a verification
		 of the DYNAMIC segment.  Still the found address is the best
		 one GDB could find.  */

	      warning (_(".dynamic section for \"%s\" "
			 "is not at the expected address "
			 "(wrong library or version mismatch?)"), so->so_name);
	    }
	}

    set_addr:
      so->lm_info->l_addr = l_addr;
      so->lm_info->l_addr_p = 1;
    }

  return so->lm_info->l_addr;
}

/* Per pspace SVR4 specific data.  */

struct svr4_info
{
  CORE_ADDR debug_base;	/* Base of dynamic linker structures.  */

  /* Validity flag for debug_loader_offset.  */
  int debug_loader_offset_p;

  /* Load address for the dynamic linker, inferred.  */
  CORE_ADDR debug_loader_offset;

  /* Name of the dynamic linker, valid if debug_loader_offset_p.  */
  char *debug_loader_name;

  /* Load map address for the main executable.  */
  CORE_ADDR main_lm_addr;

  CORE_ADDR interp_text_sect_low;
  CORE_ADDR interp_text_sect_high;
  CORE_ADDR interp_plt_sect_low;
  CORE_ADDR interp_plt_sect_high;
};

/* Per-program-space data key.  */
static const struct program_space_data *solib_svr4_pspace_data;

static void
svr4_pspace_data_cleanup (struct program_space *pspace, void *arg)
{
  struct svr4_info *info;

  info = program_space_data (pspace, solib_svr4_pspace_data);
  xfree (info);
}

/* Get the current svr4 data.  If none is found yet, add it now.  This
   function always returns a valid object.  */

static struct svr4_info *
get_svr4_info (void)
{
  struct svr4_info *info;

  info = program_space_data (current_program_space, solib_svr4_pspace_data);
  if (info != NULL)
    return info;

  info = XZALLOC (struct svr4_info);
  set_program_space_data (current_program_space, solib_svr4_pspace_data, info);
  return info;
}

/* Local function prototypes */

static int match_main (const char *);

/* Read program header TYPE from inferior memory.  The header is found
   by scanning the OS auxillary vector.

   If TYPE == -1, return the program headers instead of the contents of
   one program header.

   Return a pointer to allocated memory holding the program header contents,
   or NULL on failure.  If sucessful, and unless P_SECT_SIZE is NULL, the
   size of those contents is returned to P_SECT_SIZE.  Likewise, the target
   architecture size (32-bit or 64-bit) is returned to P_ARCH_SIZE.  */

static gdb_byte *
read_program_header (int type, int *p_sect_size, int *p_arch_size)
{
  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());
  CORE_ADDR at_phdr, at_phent, at_phnum, pt_phdr = 0;
  int arch_size, sect_size;
  CORE_ADDR sect_addr;
  gdb_byte *buf;
  int pt_phdr_p = 0;

  /* Get required auxv elements from target.  */
  if (target_auxv_search (&current_target, AT_PHDR, &at_phdr) <= 0)
    return 0;
  if (target_auxv_search (&current_target, AT_PHENT, &at_phent) <= 0)
    return 0;
  if (target_auxv_search (&current_target, AT_PHNUM, &at_phnum) <= 0)
    return 0;
  if (!at_phdr || !at_phnum)
    return 0;

  /* Determine ELF architecture type.  */
  if (at_phent == sizeof (Elf32_External_Phdr))
    arch_size = 32;
  else if (at_phent == sizeof (Elf64_External_Phdr))
    arch_size = 64;
  else
    return 0;

  /* Find the requested segment.  */
  if (type == -1)
    {
      sect_addr = at_phdr;
      sect_size = at_phent * at_phnum;
    }
  else if (arch_size == 32)
    {
      Elf32_External_Phdr phdr;
      int i;

      /* Search for requested PHDR.  */
      for (i = 0; i < at_phnum; i++)
	{
	  int p_type;

	  if (target_read_memory (at_phdr + i * sizeof (phdr),
				  (gdb_byte *)&phdr, sizeof (phdr)))
	    return 0;

	  p_type = extract_unsigned_integer ((gdb_byte *) phdr.p_type,
					     4, byte_order);

	  if (p_type == PT_PHDR)
	    {
	      pt_phdr_p = 1;
	      pt_phdr = extract_unsigned_integer ((gdb_byte *) phdr.p_vaddr,
						  4, byte_order);
	    }

	  if (p_type == type)
	    break;
	}

      if (i == at_phnum)
	return 0;

      /* Retrieve address and size.  */
      sect_addr = extract_unsigned_integer ((gdb_byte *)phdr.p_vaddr,
					    4, byte_order);
      sect_size = extract_unsigned_integer ((gdb_byte *)phdr.p_memsz,
					    4, byte_order);
    }
  else
    {
      Elf64_External_Phdr phdr;
      int i;

      /* Search for requested PHDR.  */
      for (i = 0; i < at_phnum; i++)
	{
	  int p_type;

	  if (target_read_memory (at_phdr + i * sizeof (phdr),
				  (gdb_byte *)&phdr, sizeof (phdr)))
	    return 0;

	  p_type = extract_unsigned_integer ((gdb_byte *) phdr.p_type,
					     4, byte_order);

	  if (p_type == PT_PHDR)
	    {
	      pt_phdr_p = 1;
	      pt_phdr = extract_unsigned_integer ((gdb_byte *) phdr.p_vaddr,
						  8, byte_order);
	    }

	  if (p_type == type)
	    break;
	}

      if (i == at_phnum)
	return 0;

      /* Retrieve address and size.  */
      sect_addr = extract_unsigned_integer ((gdb_byte *)phdr.p_vaddr,
					    8, byte_order);
      sect_size = extract_unsigned_integer ((gdb_byte *)phdr.p_memsz,
					    8, byte_order);
    }

  /* PT_PHDR is optional, but we really need it
     for PIE to make this work in general.  */

  if (pt_phdr_p)
    {
      /* at_phdr is real address in memory. pt_phdr is what pheader says it is.
	 Relocation offset is the difference between the two. */
      sect_addr = sect_addr + (at_phdr - pt_phdr);
    }

  /* Read in requested program header.  */
  buf = xmalloc (sect_size);
  if (target_read_memory (sect_addr, buf, sect_size))
    {
      xfree (buf);
      return NULL;
    }

  if (p_arch_size)
    *p_arch_size = arch_size;
  if (p_sect_size)
    *p_sect_size = sect_size;

  return buf;
}


/* Return program interpreter string.  */
static gdb_byte *
find_program_interpreter (void)
{
  gdb_byte *buf = NULL;

  /* If we have an exec_bfd, use its section table.  */
  if (exec_bfd
      && bfd_get_flavour (exec_bfd) == bfd_target_elf_flavour)
   {
     struct bfd_section *interp_sect;

     interp_sect = bfd_get_section_by_name (exec_bfd, ".interp");
     if (interp_sect != NULL)
      {
	int sect_size = bfd_section_size (exec_bfd, interp_sect);

	buf = xmalloc (sect_size);
	bfd_get_section_contents (exec_bfd, interp_sect, buf, 0, sect_size);
      }
   }

  /* If we didn't find it, use the target auxillary vector.  */
  if (!buf)
    buf = read_program_header (PT_INTERP, NULL, NULL);

  return buf;
}


/* Scan for DYNTAG in .dynamic section of ABFD.  If DYNTAG is found 1 is
   returned and the corresponding PTR is set.  */

static int
scan_dyntag (int dyntag, bfd *abfd, CORE_ADDR *ptr)
{
  int arch_size, step, sect_size;
  long dyn_tag;
  CORE_ADDR dyn_ptr, dyn_addr;
  gdb_byte *bufend, *bufstart, *buf;
  Elf32_External_Dyn *x_dynp_32;
  Elf64_External_Dyn *x_dynp_64;
  struct bfd_section *sect;
  struct target_section *target_section;

  if (abfd == NULL)
    return 0;

  if (bfd_get_flavour (abfd) != bfd_target_elf_flavour)
    return 0;

  arch_size = bfd_get_arch_size (abfd);
  if (arch_size == -1)
    return 0;

  /* Find the start address of the .dynamic section.  */
  sect = bfd_get_section_by_name (abfd, ".dynamic");
  if (sect == NULL)
    return 0;

  for (target_section = current_target_sections->sections;
       target_section < current_target_sections->sections_end;
       target_section++)
    if (sect == target_section->the_bfd_section)
      break;
  if (target_section < current_target_sections->sections_end)
    dyn_addr = target_section->addr;
  else
    {
      /* ABFD may come from OBJFILE acting only as a symbol file without being
	 loaded into the target (see add_symbol_file_command).  This case is
	 such fallback to the file VMA address without the possibility of
	 having the section relocated to its actual in-memory address.  */

      dyn_addr = bfd_section_vma (abfd, sect);
    }

  /* Read in .dynamic from the BFD.  We will get the actual value
     from memory later.  */
  sect_size = bfd_section_size (abfd, sect);
  buf = bufstart = alloca (sect_size);
  if (!bfd_get_section_contents (abfd, sect,
				 buf, 0, sect_size))
    return 0;

  /* Iterate over BUF and scan for DYNTAG.  If found, set PTR and return.  */
  step = (arch_size == 32) ? sizeof (Elf32_External_Dyn)
			   : sizeof (Elf64_External_Dyn);
  for (bufend = buf + sect_size;
       buf < bufend;
       buf += step)
  {
    if (arch_size == 32)
      {
	x_dynp_32 = (Elf32_External_Dyn *) buf;
	dyn_tag = bfd_h_get_32 (abfd, (bfd_byte *) x_dynp_32->d_tag);
	dyn_ptr = bfd_h_get_32 (abfd, (bfd_byte *) x_dynp_32->d_un.d_ptr);
      }
    else
      {
	x_dynp_64 = (Elf64_External_Dyn *) buf;
	dyn_tag = bfd_h_get_64 (abfd, (bfd_byte *) x_dynp_64->d_tag);
	dyn_ptr = bfd_h_get_64 (abfd, (bfd_byte *) x_dynp_64->d_un.d_ptr);
      }
     if (dyn_tag == DT_NULL)
       return 0;
     if (dyn_tag == dyntag)
       {
	 /* If requested, try to read the runtime value of this .dynamic
	    entry.  */
	 if (ptr)
	   {
	     struct type *ptr_type;
	     gdb_byte ptr_buf[8];
	     CORE_ADDR ptr_addr;

	     ptr_type = builtin_type (target_gdbarch ())->builtin_data_ptr;
	     ptr_addr = dyn_addr + (buf - bufstart) + arch_size / 8;
	     if (target_read_memory (ptr_addr, ptr_buf, arch_size / 8) == 0)
	       dyn_ptr = extract_typed_address (ptr_buf, ptr_type);
	     *ptr = dyn_ptr;
	   }
	 return 1;
       }
  }

  return 0;
}

/* Scan for DYNTAG in .dynamic section of the target's main executable,
   found by consulting the OS auxillary vector.  If DYNTAG is found 1 is
   returned and the corresponding PTR is set.  */

static int
scan_dyntag_auxv (int dyntag, CORE_ADDR *ptr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());
  int sect_size, arch_size, step;
  long dyn_tag;
  CORE_ADDR dyn_ptr;
  gdb_byte *bufend, *bufstart, *buf;

  /* Read in .dynamic section.  */
  buf = bufstart = read_program_header (PT_DYNAMIC, &sect_size, &arch_size);
  if (!buf)
    return 0;

  /* Iterate over BUF and scan for DYNTAG.  If found, set PTR and return.  */
  step = (arch_size == 32) ? sizeof (Elf32_External_Dyn)
			   : sizeof (Elf64_External_Dyn);
  for (bufend = buf + sect_size;
       buf < bufend;
       buf += step)
  {
    if (arch_size == 32)
      {
	Elf32_External_Dyn *dynp = (Elf32_External_Dyn *) buf;

	dyn_tag = extract_unsigned_integer ((gdb_byte *) dynp->d_tag,
					    4, byte_order);
	dyn_ptr = extract_unsigned_integer ((gdb_byte *) dynp->d_un.d_ptr,
					    4, byte_order);
      }
    else
      {
	Elf64_External_Dyn *dynp = (Elf64_External_Dyn *) buf;

	dyn_tag = extract_unsigned_integer ((gdb_byte *) dynp->d_tag,
					    8, byte_order);
	dyn_ptr = extract_unsigned_integer ((gdb_byte *) dynp->d_un.d_ptr,
					    8, byte_order);
      }
    if (dyn_tag == DT_NULL)
      break;

    if (dyn_tag == dyntag)
      {
	if (ptr)
	  *ptr = dyn_ptr;

	xfree (bufstart);
	return 1;
      }
  }

  xfree (bufstart);
  return 0;
}

/* Locate the base address of dynamic linker structs for SVR4 elf
   targets.

   For SVR4 elf targets the address of the dynamic linker's runtime
   structure is contained within the dynamic info section in the
   executable file.  The dynamic section is also mapped into the
   inferior address space.  Because the runtime loader fills in the
   real address before starting the inferior, we have to read in the
   dynamic info section from the inferior address space.
   If there are any errors while trying to find the address, we
   silently return 0, otherwise the found address is returned.  */

static CORE_ADDR
elf_locate_base (void)
{
  struct minimal_symbol *msymbol;
  CORE_ADDR dyn_ptr;

  /* Look for DT_MIPS_RLD_MAP first.  MIPS executables use this
     instead of DT_DEBUG, although they sometimes contain an unused
     DT_DEBUG.  */
  if (scan_dyntag (DT_MIPS_RLD_MAP, exec_bfd, &dyn_ptr)
      || scan_dyntag_auxv (DT_MIPS_RLD_MAP, &dyn_ptr))
    {
      struct type *ptr_type = builtin_type (target_gdbarch ())->builtin_data_ptr;
      gdb_byte *pbuf;
      int pbuf_size = TYPE_LENGTH (ptr_type);

      pbuf = alloca (pbuf_size);
      /* DT_MIPS_RLD_MAP contains a pointer to the address
	 of the dynamic link structure.  */
      if (target_read_memory (dyn_ptr, pbuf, pbuf_size))
	return 0;
      return extract_typed_address (pbuf, ptr_type);
    }

  /* Find DT_DEBUG.  */
  if (scan_dyntag (DT_DEBUG, exec_bfd, &dyn_ptr)
      || scan_dyntag_auxv (DT_DEBUG, &dyn_ptr))
    return dyn_ptr;

  /* This may be a static executable.  Look for the symbol
     conventionally named _r_debug, as a last resort.  */
  msymbol = lookup_minimal_symbol ("_r_debug", NULL, symfile_objfile);
  if (msymbol != NULL)
    return SYMBOL_VALUE_ADDRESS (msymbol);

  /* DT_DEBUG entry not found.  */
  return 0;
}

/* Locate the base address of dynamic linker structs.

   For both the SunOS and SVR4 shared library implementations, if the
   inferior executable has been linked dynamically, there is a single
   address somewhere in the inferior's data space which is the key to
   locating all of the dynamic linker's runtime structures.  This
   address is the value of the debug base symbol.  The job of this
   function is to find and return that address, or to return 0 if there
   is no such address (the executable is statically linked for example).

   For SunOS, the job is almost trivial, since the dynamic linker and
   all of it's structures are statically linked to the executable at
   link time.  Thus the symbol for the address we are looking for has
   already been added to the minimal symbol table for the executable's
   objfile at the time the symbol file's symbols were read, and all we
   have to do is look it up there.  Note that we explicitly do NOT want
   to find the copies in the shared library.

   The SVR4 version is a bit more complicated because the address
   is contained somewhere in the dynamic info section.  We have to go
   to a lot more work to discover the address of the debug base symbol.
   Because of this complexity, we cache the value we find and return that
   value on subsequent invocations.  Note there is no copy in the
   executable symbol tables.  */

static CORE_ADDR
locate_base (struct svr4_info *info)
{
  /* Check to see if we have a currently valid address, and if so, avoid
     doing all this work again and just return the cached address.  If
     we have no cached address, try to locate it in the dynamic info
     section for ELF executables.  There's no point in doing any of this
     though if we don't have some link map offsets to work with.  */

  if (info->debug_base == 0 && svr4_have_link_map_offsets ())
    info->debug_base = elf_locate_base ();
  return info->debug_base;
}

/* Find the first element in the inferior's dynamic link map, and
   return its address in the inferior.  Return zero if the address
   could not be determined.

   FIXME: Perhaps we should validate the info somehow, perhaps by
   checking r_version for a known version number, or r_state for
   RT_CONSISTENT.  */

static CORE_ADDR
solib_svr4_r_map (struct svr4_info *info)
{
  struct link_map_offsets *lmo = svr4_fetch_link_map_offsets ();
  struct type *ptr_type = builtin_type (target_gdbarch ())->builtin_data_ptr;
  CORE_ADDR addr = 0;
  volatile struct gdb_exception ex;

  TRY_CATCH (ex, RETURN_MASK_ERROR)
    {
      addr = read_memory_typed_address (info->debug_base + lmo->r_map_offset,
                                        ptr_type);
    }
  exception_print (gdb_stderr, ex);
  return addr;
}

/* Find r_brk from the inferior's debug base.  */

static CORE_ADDR
solib_svr4_r_brk (struct svr4_info *info)
{
  struct link_map_offsets *lmo = svr4_fetch_link_map_offsets ();
  struct type *ptr_type = builtin_type (target_gdbarch ())->builtin_data_ptr;

  return read_memory_typed_address (info->debug_base + lmo->r_brk_offset,
				    ptr_type);
}

/* Find the link map for the dynamic linker (if it is not in the
   normal list of loaded shared objects).  */

static CORE_ADDR
solib_svr4_r_ldsomap (struct svr4_info *info)
{
  struct link_map_offsets *lmo = svr4_fetch_link_map_offsets ();
  struct type *ptr_type = builtin_type (target_gdbarch ())->builtin_data_ptr;
  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());
  ULONGEST version;

  /* Check version, and return zero if `struct r_debug' doesn't have
     the r_ldsomap member.  */
  version
    = read_memory_unsigned_integer (info->debug_base + lmo->r_version_offset,
				    lmo->r_version_size, byte_order);
  if (version < 2 || lmo->r_ldsomap_offset == -1)
    return 0;

  return read_memory_typed_address (info->debug_base + lmo->r_ldsomap_offset,
				    ptr_type);
}

/* On Solaris systems with some versions of the dynamic linker,
   ld.so's l_name pointer points to the SONAME in the string table
   rather than into writable memory.  So that GDB can find shared
   libraries when loading a core file generated by gcore, ensure that
   memory areas containing the l_name string are saved in the core
   file.  */

static int
svr4_keep_data_in_core (CORE_ADDR vaddr, unsigned long size)
{
  struct svr4_info *info;
  CORE_ADDR ldsomap;
  struct so_list *new;
  struct cleanup *old_chain;
  CORE_ADDR name_lm;

  info = get_svr4_info ();

  info->debug_base = 0;
  locate_base (info);
  if (!info->debug_base)
    return 0;

  ldsomap = solib_svr4_r_ldsomap (info);
  if (!ldsomap)
    return 0;

  new = XZALLOC (struct so_list);
  old_chain = make_cleanup (xfree, new);
  new->lm_info = lm_info_read (ldsomap);
  make_cleanup (xfree, new->lm_info);
  name_lm = new->lm_info ? new->lm_info->l_name : 0;
  do_cleanups (old_chain);

  return (name_lm >= vaddr && name_lm < vaddr + size);
}

/* Implement the "open_symbol_file_object" target_so_ops method.

   If no open symbol file, attempt to locate and open the main symbol
   file.  On SVR4 systems, this is the first link map entry.  If its
   name is here, we can open it.  Useful when attaching to a process
   without first loading its symbol file.  */

static int
open_symbol_file_object (void *from_ttyp)
{
  CORE_ADDR lm, l_name;
  char *filename;
  int errcode;
  int from_tty = *(int *)from_ttyp;
  struct link_map_offsets *lmo = svr4_fetch_link_map_offsets ();
  struct type *ptr_type = builtin_type (target_gdbarch ())->builtin_data_ptr;
  int l_name_size = TYPE_LENGTH (ptr_type);
  gdb_byte *l_name_buf = xmalloc (l_name_size);
  struct cleanup *cleanups = make_cleanup (xfree, l_name_buf);
  struct svr4_info *info = get_svr4_info ();

  if (symfile_objfile)
    if (!query (_("Attempt to reload symbols from process? ")))
      {
	do_cleanups (cleanups);
	return 0;
      }

  /* Always locate the debug struct, in case it has moved.  */
  info->debug_base = 0;
  if (locate_base (info) == 0)
    {
      do_cleanups (cleanups);
      return 0;	/* failed somehow...  */
    }

  /* First link map member should be the executable.  */
  lm = solib_svr4_r_map (info);
  if (lm == 0)
    {
      do_cleanups (cleanups);
      return 0;	/* failed somehow...  */
    }

  /* Read address of name from target memory to GDB.  */
  read_memory (lm + lmo->l_name_offset, l_name_buf, l_name_size);

  /* Convert the address to host format.  */
  l_name = extract_typed_address (l_name_buf, ptr_type);

  if (l_name == 0)
    {
      do_cleanups (cleanups);
      return 0;		/* No filename.  */
    }

  /* Now fetch the filename from target memory.  */
  target_read_string (l_name, &filename, SO_NAME_MAX_PATH_SIZE - 1, &errcode);
  make_cleanup (xfree, filename);

  if (errcode)
    {
      warning (_("failed to read exec filename from attached file: %s"),
	       safe_strerror (errcode));
      do_cleanups (cleanups);
      return 0;
    }

  /* Have a pathname: read the symbol file.  */
  symbol_file_add_main (filename, from_tty);

  do_cleanups (cleanups);
  return 1;
}

/* Data exchange structure for the XML parser as returned by
   svr4_current_sos_via_xfer_libraries.  */

struct svr4_library_list
{
  struct so_list *head, **tailp;

  /* Inferior address of struct link_map used for the main executable.  It is
     NULL if not known.  */
  CORE_ADDR main_lm;
};

/* Implementation for target_so_ops.free_so.  */

static void
svr4_free_so (struct so_list *so)
{
  xfree (so->lm_info);
}

/* Free so_list built so far (called via cleanup).  */

static void
svr4_free_library_list (void *p_list)
{
  struct so_list *list = *(struct so_list **) p_list;

  while (list != NULL)
    {
      struct so_list *next = list->next;

      free_so (list);
      list = next;
    }
}

#ifdef HAVE_LIBEXPAT

#include "xml-support.h"

/* Handle the start of a <library> element.  Note: new elements are added
   at the tail of the list, keeping the list in order.  */

static void
library_list_start_library (struct gdb_xml_parser *parser,
			    const struct gdb_xml_element *element,
			    void *user_data, VEC(gdb_xml_value_s) *attributes)
{
  struct svr4_library_list *list = user_data;
  const char *name = xml_find_attribute (attributes, "name")->value;
  ULONGEST *lmp = xml_find_attribute (attributes, "lm")->value;
  ULONGEST *l_addrp = xml_find_attribute (attributes, "l_addr")->value;
  ULONGEST *l_ldp = xml_find_attribute (attributes, "l_ld")->value;
  struct so_list *new_elem;

  new_elem = XZALLOC (struct so_list);
  new_elem->lm_info = XZALLOC (struct lm_info);
  new_elem->lm_info->lm_addr = *lmp;
  new_elem->lm_info->l_addr_inferior = *l_addrp;
  new_elem->lm_info->l_ld = *l_ldp;

  strncpy (new_elem->so_name, name, sizeof (new_elem->so_name) - 1);
  new_elem->so_name[sizeof (new_elem->so_name) - 1] = 0;
  strcpy (new_elem->so_original_name, new_elem->so_name);

  *list->tailp = new_elem;
  list->tailp = &new_elem->next;
}

/* Handle the start of a <library-list-svr4> element.  */

static void
svr4_library_list_start_list (struct gdb_xml_parser *parser,
			      const struct gdb_xml_element *element,
			      void *user_data, VEC(gdb_xml_value_s) *attributes)
{
  struct svr4_library_list *list = user_data;
  const char *version = xml_find_attribute (attributes, "version")->value;
  struct gdb_xml_value *main_lm = xml_find_attribute (attributes, "main-lm");

  if (strcmp (version, "1.0") != 0)
    gdb_xml_error (parser,
		   _("SVR4 Library list has unsupported version \"%s\""),
		   version);

  if (main_lm)
    list->main_lm = *(ULONGEST *) main_lm->value;
}

/* The allowed elements and attributes for an XML library list.
   The root element is a <library-list>.  */

static const struct gdb_xml_attribute svr4_library_attributes[] =
{
  { "name", GDB_XML_AF_NONE, NULL, NULL },
  { "lm", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL },
  { "l_addr", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL },
  { "l_ld", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_element svr4_library_list_children[] =
{
  {
    "library", svr4_library_attributes, NULL,
    GDB_XML_EF_REPEATABLE | GDB_XML_EF_OPTIONAL,
    library_list_start_library, NULL
  },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

static const struct gdb_xml_attribute svr4_library_list_attributes[] =
{
  { "version", GDB_XML_AF_NONE, NULL, NULL },
  { "main-lm", GDB_XML_AF_OPTIONAL, gdb_xml_parse_attr_ulongest, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_element svr4_library_list_elements[] =
{
  { "library-list-svr4", svr4_library_list_attributes, svr4_library_list_children,
    GDB_XML_EF_NONE, svr4_library_list_start_list, NULL },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

/* Parse qXfer:libraries:read packet into *SO_LIST_RETURN.  Return 1 if

   Return 0 if packet not supported, *SO_LIST_RETURN is not modified in such
   case.  Return 1 if *SO_LIST_RETURN contains the library list, it may be
   empty, caller is responsible for freeing all its entries.  */

static int
svr4_parse_libraries (const char *document, struct svr4_library_list *list)
{
  struct cleanup *back_to = make_cleanup (svr4_free_library_list,
					  &list->head);

  memset (list, 0, sizeof (*list));
  list->tailp = &list->head;
  if (gdb_xml_parse_quick (_("target library list"), "library-list.dtd",
			   svr4_library_list_elements, document, list) == 0)
    {
      /* Parsed successfully, keep the result.  */
      discard_cleanups (back_to);
      return 1;
    }

  do_cleanups (back_to);
  return 0;
}

/* Attempt to get so_list from target via qXfer:libraries:read packet.

   Return 0 if packet not supported, *SO_LIST_RETURN is not modified in such
   case.  Return 1 if *SO_LIST_RETURN contains the library list, it may be
   empty, caller is responsible for freeing all its entries.  */

static int
svr4_current_sos_via_xfer_libraries (struct svr4_library_list *list)
{
  char *svr4_library_document;
  int result;
  struct cleanup *back_to;

  /* Fetch the list of shared libraries.  */
  svr4_library_document = target_read_stralloc (&current_target,
						TARGET_OBJECT_LIBRARIES_SVR4,
						NULL);
  if (svr4_library_document == NULL)
    return 0;

  back_to = make_cleanup (xfree, svr4_library_document);
  result = svr4_parse_libraries (svr4_library_document, list);
  do_cleanups (back_to);

  return result;
}

#else

static int
svr4_current_sos_via_xfer_libraries (struct svr4_library_list *list)
{
  return 0;
}

#endif

/* If no shared library information is available from the dynamic
   linker, build a fallback list from other sources.  */

static struct so_list *
svr4_default_sos (void)
{
  struct svr4_info *info = get_svr4_info ();
  struct so_list *new;

  if (!info->debug_loader_offset_p)
    return NULL;

  new = XZALLOC (struct so_list);

  new->lm_info = xzalloc (sizeof (struct lm_info));

  /* Nothing will ever check the other fields if we set l_addr_p.  */
  new->lm_info->l_addr = info->debug_loader_offset;
  new->lm_info->l_addr_p = 1;

  strncpy (new->so_name, info->debug_loader_name, SO_NAME_MAX_PATH_SIZE - 1);
  new->so_name[SO_NAME_MAX_PATH_SIZE - 1] = '\0';
  strcpy (new->so_original_name, new->so_name);

  return new;
}

/* Read the whole inferior libraries chain starting at address LM.  Add the
   entries to the tail referenced by LINK_PTR_PTR.  Ignore the first entry if
   IGNORE_FIRST and set global MAIN_LM_ADDR according to it.  */

static void
svr4_read_so_list (CORE_ADDR lm, struct so_list ***link_ptr_ptr,
		   int ignore_first)
{
  CORE_ADDR prev_lm = 0, next_lm;

  for (; lm != 0; prev_lm = lm, lm = next_lm)
    {
      struct so_list *new;
      struct cleanup *old_chain;
      int errcode;
      char *buffer;

      new = XZALLOC (struct so_list);
      old_chain = make_cleanup_free_so (new);

      new->lm_info = lm_info_read (lm);
      if (new->lm_info == NULL)
	{
	  do_cleanups (old_chain);
	  break;
	}

      next_lm = new->lm_info->l_next;

      if (new->lm_info->l_prev != prev_lm)
	{
	  warning (_("Corrupted shared library list: %s != %s"),
		   paddress (target_gdbarch (), prev_lm),
		   paddress (target_gdbarch (), new->lm_info->l_prev));
	  do_cleanups (old_chain);
	  break;
	}

      /* For SVR4 versions, the first entry in the link map is for the
         inferior executable, so we must ignore it.  For some versions of
         SVR4, it has no name.  For others (Solaris 2.3 for example), it
         does have a name, so we can no longer use a missing name to
         decide when to ignore it.  */
      if (ignore_first && new->lm_info->l_prev == 0)
	{
	  struct svr4_info *info = get_svr4_info ();

	  info->main_lm_addr = new->lm_info->lm_addr;
	  do_cleanups (old_chain);
	  continue;
	}

      /* Extract this shared object's name.  */
      target_read_string (new->lm_info->l_name, &buffer,
			  SO_NAME_MAX_PATH_SIZE - 1, &errcode);
      if (errcode != 0)
	{
	  warning (_("Can't read pathname for load map: %s."),
		   safe_strerror (errcode));
	  do_cleanups (old_chain);
	  continue;
	}

      strncpy (new->so_name, buffer, SO_NAME_MAX_PATH_SIZE - 1);
      new->so_name[SO_NAME_MAX_PATH_SIZE - 1] = '\0';
      strcpy (new->so_original_name, new->so_name);
      xfree (buffer);

      /* If this entry has no name, or its name matches the name
	 for the main executable, don't include it in the list.  */
      if (! new->so_name[0] || match_main (new->so_name))
	{
	  do_cleanups (old_chain);
	  continue;
	}

      discard_cleanups (old_chain);
      new->next = 0;
      **link_ptr_ptr = new;
      *link_ptr_ptr = &new->next;
    }
}

/* Implement the "current_sos" target_so_ops method.  */

static struct so_list *
svr4_current_sos (void)
{
  CORE_ADDR lm;
  struct so_list *head = NULL;
  struct so_list **link_ptr = &head;
  struct svr4_info *info;
  struct cleanup *back_to;
  int ignore_first;
  struct svr4_library_list library_list;

  /* Fall back to manual examination of the target if the packet is not
     supported or gdbserver failed to find DT_DEBUG.  gdb.server/solib-list.exp
     tests a case where gdbserver cannot find the shared libraries list while
     GDB itself is able to find it via SYMFILE_OBJFILE.

     Unfortunately statically linked inferiors will also fall back through this
     suboptimal code path.  */

  if (svr4_current_sos_via_xfer_libraries (&library_list))
    {
      if (library_list.main_lm)
	{
	  info = get_svr4_info ();
	  info->main_lm_addr = library_list.main_lm;
	}

      return library_list.head ? library_list.head : svr4_default_sos ();
    }

  info = get_svr4_info ();

  /* Always locate the debug struct, in case it has moved.  */
  info->debug_base = 0;
  locate_base (info);

  /* If we can't find the dynamic linker's base structure, this
     must not be a dynamically linked executable.  Hmm.  */
  if (! info->debug_base)
    return svr4_default_sos ();

  /* Assume that everything is a library if the dynamic loader was loaded
     late by a static executable.  */
  if (exec_bfd && bfd_get_section_by_name (exec_bfd, ".dynamic") == NULL)
    ignore_first = 0;
  else
    ignore_first = 1;

  back_to = make_cleanup (svr4_free_library_list, &head);

  /* Walk the inferior's link map list, and build our list of
     `struct so_list' nodes.  */
  lm = solib_svr4_r_map (info);
  if (lm)
    svr4_read_so_list (lm, &link_ptr, ignore_first);

  /* On Solaris, the dynamic linker is not in the normal list of
     shared objects, so make sure we pick it up too.  Having
     symbol information for the dynamic linker is quite crucial
     for skipping dynamic linker resolver code.  */
  lm = solib_svr4_r_ldsomap (info);
  if (lm)
    svr4_read_so_list (lm, &link_ptr, 0);

  discard_cleanups (back_to);

  if (head == NULL)
    return svr4_default_sos ();

  return head;
}

/* Get the address of the link_map for a given OBJFILE.  */

CORE_ADDR
svr4_fetch_objfile_link_map (struct objfile *objfile)
{
  struct so_list *so;
  struct svr4_info *info = get_svr4_info ();

  /* Cause svr4_current_sos() to be run if it hasn't been already.  */
  if (info->main_lm_addr == 0)
    solib_add (NULL, 0, &current_target, auto_solib_add);

  /* svr4_current_sos() will set main_lm_addr for the main executable.  */
  if (objfile == symfile_objfile)
    return info->main_lm_addr;

  /* The other link map addresses may be found by examining the list
     of shared libraries.  */
  for (so = master_so_list (); so; so = so->next)
    if (so->objfile == objfile)
      return so->lm_info->lm_addr;

  /* Not found!  */
  return 0;
}

/* On some systems, the only way to recognize the link map entry for
   the main executable file is by looking at its name.  Return
   non-zero iff SONAME matches one of the known main executable names.  */

static int
match_main (const char *soname)
{
  const char * const *mainp;

  for (mainp = main_name_list; *mainp != NULL; mainp++)
    {
      if (strcmp (soname, *mainp) == 0)
	return (1);
    }

  return (0);
}

/* Return 1 if PC lies in the dynamic symbol resolution code of the
   SVR4 run time loader.  */

int
svr4_in_dynsym_resolve_code (CORE_ADDR pc)
{
  struct svr4_info *info = get_svr4_info ();

  return ((pc >= info->interp_text_sect_low
	   && pc < info->interp_text_sect_high)
	  || (pc >= info->interp_plt_sect_low
	      && pc < info->interp_plt_sect_high)
	  || in_plt_section (pc, NULL)
	  || in_gnu_ifunc_stub (pc));
}

/* Given an executable's ABFD and target, compute the entry-point
   address.  */

static CORE_ADDR
exec_entry_point (struct bfd *abfd, struct target_ops *targ)
{
  CORE_ADDR addr;

  /* KevinB wrote ... for most targets, the address returned by
     bfd_get_start_address() is the entry point for the start
     function.  But, for some targets, bfd_get_start_address() returns
     the address of a function descriptor from which the entry point
     address may be extracted.  This address is extracted by
     gdbarch_convert_from_func_ptr_addr().  The method
     gdbarch_convert_from_func_ptr_addr() is the merely the identify
     function for targets which don't use function descriptors.  */
  addr = gdbarch_convert_from_func_ptr_addr (target_gdbarch (),
					     bfd_get_start_address (abfd),
					     targ);
  return gdbarch_addr_bits_remove (target_gdbarch (), addr);
}

/* Helper function for gdb_bfd_lookup_symbol.  */

static int
cmp_name_and_sec_flags (asymbol *sym, void *data)
{
  return (strcmp (sym->name, (const char *) data) == 0
	  && (sym->section->flags & (SEC_CODE | SEC_DATA)) != 0);
}
/* Arrange for dynamic linker to hit breakpoint.

   Both the SunOS and the SVR4 dynamic linkers have, as part of their
   debugger interface, support for arranging for the inferior to hit
   a breakpoint after mapping in the shared libraries.  This function
   enables that breakpoint.

   For SunOS, there is a special flag location (in_debugger) which we
   set to 1.  When the dynamic linker sees this flag set, it will set
   a breakpoint at a location known only to itself, after saving the
   original contents of that place and the breakpoint address itself,
   in it's own internal structures.  When we resume the inferior, it
   will eventually take a SIGTRAP when it runs into the breakpoint.
   We handle this (in a different place) by restoring the contents of
   the breakpointed location (which is only known after it stops),
   chasing around to locate the shared libraries that have been
   loaded, then resuming.

   For SVR4, the debugger interface structure contains a member (r_brk)
   which is statically initialized at the time the shared library is
   built, to the offset of a function (_r_debug_state) which is guaran-
   teed to be called once before mapping in a library, and again when
   the mapping is complete.  At the time we are examining this member,
   it contains only the unrelocated offset of the function, so we have
   to do our own relocation.  Later, when the dynamic linker actually
   runs, it relocates r_brk to be the actual address of _r_debug_state().

   The debugger interface structure also contains an enumeration which
   is set to either RT_ADD or RT_DELETE prior to changing the mapping,
   depending upon whether or not the library is being mapped or unmapped,
   and then set to RT_CONSISTENT after the library is mapped/unmapped.  */

static int
enable_break (struct svr4_info *info, int from_tty)
{
  struct minimal_symbol *msymbol;
  const char * const *bkpt_namep;
  asection *interp_sect;
  gdb_byte *interp_name;
  CORE_ADDR sym_addr;

  info->interp_text_sect_low = info->interp_text_sect_high = 0;
  info->interp_plt_sect_low = info->interp_plt_sect_high = 0;

  /* If we already have a shared library list in the target, and
     r_debug contains r_brk, set the breakpoint there - this should
     mean r_brk has already been relocated.  Assume the dynamic linker
     is the object containing r_brk.  */

  solib_add (NULL, from_tty, &current_target, auto_solib_add);
  sym_addr = 0;
  if (info->debug_base && solib_svr4_r_map (info) != 0)
    sym_addr = solib_svr4_r_brk (info);

  if (sym_addr != 0)
    {
      struct obj_section *os;

      sym_addr = gdbarch_addr_bits_remove
	(target_gdbarch (), gdbarch_convert_from_func_ptr_addr (target_gdbarch (),
							     sym_addr,
							     &current_target));

      /* On at least some versions of Solaris there's a dynamic relocation
	 on _r_debug.r_brk and SYM_ADDR may not be relocated yet, e.g., if
	 we get control before the dynamic linker has self-relocated.
	 Check if SYM_ADDR is in a known section, if it is assume we can
	 trust its value.  This is just a heuristic though, it could go away
	 or be replaced if it's getting in the way.

	 On ARM we need to know whether the ISA of rtld_db_dlactivity (or
	 however it's spelled in your particular system) is ARM or Thumb.
	 That knowledge is encoded in the address, if it's Thumb the low bit
	 is 1.  However, we've stripped that info above and it's not clear
	 what all the consequences are of passing a non-addr_bits_remove'd
	 address to create_solib_event_breakpoint.  The call to
	 find_pc_section verifies we know about the address and have some
	 hope of computing the right kind of breakpoint to use (via
	 symbol info).  It does mean that GDB needs to be pointed at a
	 non-stripped version of the dynamic linker in order to obtain
	 information it already knows about.  Sigh.  */

      os = find_pc_section (sym_addr);
      if (os != NULL)
	{
	  /* Record the relocated start and end address of the dynamic linker
	     text and plt section for svr4_in_dynsym_resolve_code.  */
	  bfd *tmp_bfd;
	  CORE_ADDR load_addr;

	  tmp_bfd = os->objfile->obfd;
	  load_addr = ANOFFSET (os->objfile->section_offsets,
				SECT_OFF_TEXT (os->objfile));

	  interp_sect = bfd_get_section_by_name (tmp_bfd, ".text");
	  if (interp_sect)
	    {
	      info->interp_text_sect_low =
		bfd_section_vma (tmp_bfd, interp_sect) + load_addr;
	      info->interp_text_sect_high =
		info->interp_text_sect_low
		+ bfd_section_size (tmp_bfd, interp_sect);
	    }
	  interp_sect = bfd_get_section_by_name (tmp_bfd, ".plt");
	  if (interp_sect)
	    {
	      info->interp_plt_sect_low =
		bfd_section_vma (tmp_bfd, interp_sect) + load_addr;
	      info->interp_plt_sect_high =
		info->interp_plt_sect_low
		+ bfd_section_size (tmp_bfd, interp_sect);
	    }

	  create_solib_event_breakpoint (target_gdbarch (), sym_addr);
	  return 1;
	}
    }

  /* Find the program interpreter; if not found, warn the user and drop
     into the old breakpoint at symbol code.  */
  interp_name = find_program_interpreter ();
  if (interp_name)
    {
      CORE_ADDR load_addr = 0;
      int load_addr_found = 0;
      int loader_found_in_list = 0;
      struct so_list *so;
      bfd *tmp_bfd = NULL;
      struct target_ops *tmp_bfd_target;
      volatile struct gdb_exception ex;

      sym_addr = 0;

      /* Now we need to figure out where the dynamic linker was
         loaded so that we can load its symbols and place a breakpoint
         in the dynamic linker itself.

         This address is stored on the stack.  However, I've been unable
         to find any magic formula to find it for Solaris (appears to
         be trivial on GNU/Linux).  Therefore, we have to try an alternate
         mechanism to find the dynamic linker's base address.  */

      TRY_CATCH (ex, RETURN_MASK_ALL)
        {
	  tmp_bfd = solib_bfd_open (interp_name);
	}
      if (tmp_bfd == NULL)
	goto bkpt_at_symbol;

      /* Now convert the TMP_BFD into a target.  That way target, as
         well as BFD operations can be used.  */
      tmp_bfd_target = target_bfd_reopen (tmp_bfd);
      /* target_bfd_reopen acquired its own reference, so we can
         release ours now.  */
      gdb_bfd_unref (tmp_bfd);

      /* On a running target, we can get the dynamic linker's base
         address from the shared library table.  */
      so = master_so_list ();
      while (so)
	{
	  if (svr4_same_1 (interp_name, so->so_original_name))
	    {
	      load_addr_found = 1;
	      loader_found_in_list = 1;
	      load_addr = lm_addr_check (so, tmp_bfd);
	      break;
	    }
	  so = so->next;
	}

      /* If we were not able to find the base address of the loader
         from our so_list, then try using the AT_BASE auxilliary entry.  */
      if (!load_addr_found)
        if (target_auxv_search (&current_target, AT_BASE, &load_addr) > 0)
	  {
	    int addr_bit = gdbarch_addr_bit (target_gdbarch ());

	    /* Ensure LOAD_ADDR has proper sign in its possible upper bits so
	       that `+ load_addr' will overflow CORE_ADDR width not creating
	       invalid addresses like 0x101234567 for 32bit inferiors on 64bit
	       GDB.  */

	    if (addr_bit < (sizeof (CORE_ADDR) * HOST_CHAR_BIT))
	      {
		CORE_ADDR space_size = (CORE_ADDR) 1 << addr_bit;
		CORE_ADDR tmp_entry_point = exec_entry_point (tmp_bfd,
							      tmp_bfd_target);

		gdb_assert (load_addr < space_size);

		/* TMP_ENTRY_POINT exceeding SPACE_SIZE would be for prelinked
		   64bit ld.so with 32bit executable, it should not happen.  */

		if (tmp_entry_point < space_size
		    && tmp_entry_point + load_addr >= space_size)
		  load_addr -= space_size;
	      }

	    load_addr_found = 1;
	  }

      /* Otherwise we find the dynamic linker's base address by examining
	 the current pc (which should point at the entry point for the
	 dynamic linker) and subtracting the offset of the entry point.

         This is more fragile than the previous approaches, but is a good
         fallback method because it has actually been working well in
         most cases.  */
      if (!load_addr_found)
	{
	  struct regcache *regcache
	    = get_thread_arch_regcache (inferior_ptid, target_gdbarch ());

	  load_addr = (regcache_read_pc (regcache)
		       - exec_entry_point (tmp_bfd, tmp_bfd_target));
	}

      if (!loader_found_in_list)
	{
	  info->debug_loader_name = xstrdup (interp_name);
	  info->debug_loader_offset_p = 1;
	  info->debug_loader_offset = load_addr;
	  solib_add (NULL, from_tty, &current_target, auto_solib_add);
	}

      /* Record the relocated start and end address of the dynamic linker
         text and plt section for svr4_in_dynsym_resolve_code.  */
      interp_sect = bfd_get_section_by_name (tmp_bfd, ".text");
      if (interp_sect)
	{
	  info->interp_text_sect_low =
	    bfd_section_vma (tmp_bfd, interp_sect) + load_addr;
	  info->interp_text_sect_high =
	    info->interp_text_sect_low
	    + bfd_section_size (tmp_bfd, interp_sect);
	}
      interp_sect = bfd_get_section_by_name (tmp_bfd, ".plt");
      if (interp_sect)
	{
	  info->interp_plt_sect_low =
	    bfd_section_vma (tmp_bfd, interp_sect) + load_addr;
	  info->interp_plt_sect_high =
	    info->interp_plt_sect_low
	    + bfd_section_size (tmp_bfd, interp_sect);
	}

      /* Now try to set a breakpoint in the dynamic linker.  */
      for (bkpt_namep = solib_break_names; *bkpt_namep != NULL; bkpt_namep++)
	{
	  sym_addr = gdb_bfd_lookup_symbol (tmp_bfd, cmp_name_and_sec_flags,
					    (void *) *bkpt_namep);
	  if (sym_addr != 0)
	    break;
	}

      if (sym_addr != 0)
	/* Convert 'sym_addr' from a function pointer to an address.
	   Because we pass tmp_bfd_target instead of the current
	   target, this will always produce an unrelocated value.  */
	sym_addr = gdbarch_convert_from_func_ptr_addr (target_gdbarch (),
						       sym_addr,
						       tmp_bfd_target);

      /* We're done with both the temporary bfd and target.  Closing
         the target closes the underlying bfd, because it holds the
         only remaining reference.  */
      target_close (tmp_bfd_target, 0);

      if (sym_addr != 0)
	{
	  create_solib_event_breakpoint (target_gdbarch (), load_addr + sym_addr);
	  xfree (interp_name);
	  return 1;
	}

      /* For whatever reason we couldn't set a breakpoint in the dynamic
         linker.  Warn and drop into the old code.  */
    bkpt_at_symbol:
      xfree (interp_name);
      warning (_("Unable to find dynamic linker breakpoint function.\n"
               "GDB will be unable to debug shared library initializers\n"
               "and track explicitly loaded dynamic code."));
    }

  /* Scan through the lists of symbols, trying to look up the symbol and
     set a breakpoint there.  Terminate loop when we/if we succeed.  */

  for (bkpt_namep = solib_break_names; *bkpt_namep != NULL; bkpt_namep++)
    {
      msymbol = lookup_minimal_symbol (*bkpt_namep, NULL, symfile_objfile);
      if ((msymbol != NULL) && (SYMBOL_VALUE_ADDRESS (msymbol) != 0))
	{
	  sym_addr = SYMBOL_VALUE_ADDRESS (msymbol);
	  sym_addr = gdbarch_convert_from_func_ptr_addr (target_gdbarch (),
							 sym_addr,
							 &current_target);
	  create_solib_event_breakpoint (target_gdbarch (), sym_addr);
	  return 1;
	}
    }

  if (interp_name != NULL && !current_inferior ()->attach_flag)
    {
      for (bkpt_namep = bkpt_names; *bkpt_namep != NULL; bkpt_namep++)
	{
	  msymbol = lookup_minimal_symbol (*bkpt_namep, NULL, symfile_objfile);
	  if ((msymbol != NULL) && (SYMBOL_VALUE_ADDRESS (msymbol) != 0))
	    {
	      sym_addr = SYMBOL_VALUE_ADDRESS (msymbol);
	      sym_addr = gdbarch_convert_from_func_ptr_addr (target_gdbarch (),
							     sym_addr,
							     &current_target);
	      create_solib_event_breakpoint (target_gdbarch (), sym_addr);
	      return 1;
	    }
	}
    }
  return 0;
}

/* Implement the "special_symbol_handling" target_so_ops method.  */

static void
svr4_special_symbol_handling (void)
{
  /* Nothing to do.  */
}

/* Read the ELF program headers from ABFD.  Return the contents and
   set *PHDRS_SIZE to the size of the program headers.  */

static gdb_byte *
read_program_headers_from_bfd (bfd *abfd, int *phdrs_size)
{
  Elf_Internal_Ehdr *ehdr;
  gdb_byte *buf;

  ehdr = elf_elfheader (abfd);

  *phdrs_size = ehdr->e_phnum * ehdr->e_phentsize;
  if (*phdrs_size == 0)
    return NULL;

  buf = xmalloc (*phdrs_size);
  if (bfd_seek (abfd, ehdr->e_phoff, SEEK_SET) != 0
      || bfd_bread (buf, *phdrs_size, abfd) != *phdrs_size)
    {
      xfree (buf);
      return NULL;
    }

  return buf;
}

/* Return 1 and fill *DISPLACEMENTP with detected PIE offset of inferior
   exec_bfd.  Otherwise return 0.

   We relocate all of the sections by the same amount.  This
   behavior is mandated by recent editions of the System V ABI.
   According to the System V Application Binary Interface,
   Edition 4.1, page 5-5:

     ...  Though the system chooses virtual addresses for
     individual processes, it maintains the segments' relative
     positions.  Because position-independent code uses relative
     addressesing between segments, the difference between
     virtual addresses in memory must match the difference
     between virtual addresses in the file.  The difference
     between the virtual address of any segment in memory and
     the corresponding virtual address in the file is thus a
     single constant value for any one executable or shared
     object in a given process.  This difference is the base
     address.  One use of the base address is to relocate the
     memory image of the program during dynamic linking.

   The same language also appears in Edition 4.0 of the System V
   ABI and is left unspecified in some of the earlier editions.

   Decide if the objfile needs to be relocated.  As indicated above, we will
   only be here when execution is stopped.  But during attachment PC can be at
   arbitrary address therefore regcache_read_pc can be misleading (contrary to
   the auxv AT_ENTRY value).  Moreover for executable with interpreter section
   regcache_read_pc would point to the interpreter and not the main executable.

   So, to summarize, relocations are necessary when the start address obtained
   from the executable is different from the address in auxv AT_ENTRY entry.

   [ The astute reader will note that we also test to make sure that
     the executable in question has the DYNAMIC flag set.  It is my
     opinion that this test is unnecessary (undesirable even).  It
     was added to avoid inadvertent relocation of an executable
     whose e_type member in the ELF header is not ET_DYN.  There may
     be a time in the future when it is desirable to do relocations
     on other types of files as well in which case this condition
     should either be removed or modified to accomodate the new file
     type.  - Kevin, Nov 2000. ]  */

static int
svr4_exec_displacement (CORE_ADDR *displacementp)
{
  /* ENTRY_POINT is a possible function descriptor - before
     a call to gdbarch_convert_from_func_ptr_addr.  */
  CORE_ADDR entry_point, displacement;

  if (exec_bfd == NULL)
    return 0;

  /* Therefore for ELF it is ET_EXEC and not ET_DYN.  Both shared libraries
     being executed themselves and PIE (Position Independent Executable)
     executables are ET_DYN.  */

  if ((bfd_get_file_flags (exec_bfd) & DYNAMIC) == 0)
    return 0;

  if (target_auxv_search (&current_target, AT_ENTRY, &entry_point) <= 0)
    return 0;

  displacement = entry_point - bfd_get_start_address (exec_bfd);

  /* Verify the DISPLACEMENT candidate complies with the required page
     alignment.  It is cheaper than the program headers comparison below.  */

  if (bfd_get_flavour (exec_bfd) == bfd_target_elf_flavour)
    {
      const struct elf_backend_data *elf = get_elf_backend_data (exec_bfd);

      /* p_align of PT_LOAD segments does not specify any alignment but
	 only congruency of addresses:
	   p_offset % p_align == p_vaddr % p_align
	 Kernel is free to load the executable with lower alignment.  */

      if ((displacement & (elf->minpagesize - 1)) != 0)
	return 0;
    }

  /* Verify that the auxilliary vector describes the same file as exec_bfd, by
     comparing their program headers.  If the program headers in the auxilliary
     vector do not match the program headers in the executable, then we are
     looking at a different file than the one used by the kernel - for
     instance, "gdb program" connected to "gdbserver :PORT ld.so program".  */

  if (bfd_get_flavour (exec_bfd) == bfd_target_elf_flavour)
    {
      /* Be optimistic and clear OK only if GDB was able to verify the headers
	 really do not match.  */
      int phdrs_size, phdrs2_size, ok = 1;
      gdb_byte *buf, *buf2;
      int arch_size;

      buf = read_program_header (-1, &phdrs_size, &arch_size);
      buf2 = read_program_headers_from_bfd (exec_bfd, &phdrs2_size);
      if (buf != NULL && buf2 != NULL)
	{
	  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());

	  /* We are dealing with three different addresses.  EXEC_BFD
	     represents current address in on-disk file.  target memory content
	     may be different from EXEC_BFD as the file may have been prelinked
	     to a different address after the executable has been loaded.
	     Moreover the address of placement in target memory can be
	     different from what the program headers in target memory say -
	     this is the goal of PIE.

	     Detected DISPLACEMENT covers both the offsets of PIE placement and
	     possible new prelink performed after start of the program.  Here
	     relocate BUF and BUF2 just by the EXEC_BFD vs. target memory
	     content offset for the verification purpose.  */

	  if (phdrs_size != phdrs2_size
	      || bfd_get_arch_size (exec_bfd) != arch_size)
	    ok = 0;
	  else if (arch_size == 32
		   && phdrs_size >= sizeof (Elf32_External_Phdr)
	           && phdrs_size % sizeof (Elf32_External_Phdr) == 0)
	    {
	      Elf_Internal_Ehdr *ehdr2 = elf_tdata (exec_bfd)->elf_header;
	      Elf_Internal_Phdr *phdr2 = elf_tdata (exec_bfd)->phdr;
	      CORE_ADDR displacement = 0;
	      int i;

	      /* DISPLACEMENT could be found more easily by the difference of
		 ehdr2->e_entry.  But we haven't read the ehdr yet, and we
		 already have enough information to compute that displacement
		 with what we've read.  */

	      for (i = 0; i < ehdr2->e_phnum; i++)
		if (phdr2[i].p_type == PT_LOAD)
		  {
		    Elf32_External_Phdr *phdrp;
		    gdb_byte *buf_vaddr_p, *buf_paddr_p;
		    CORE_ADDR vaddr, paddr;
		    CORE_ADDR displacement_vaddr = 0;
		    CORE_ADDR displacement_paddr = 0;

		    phdrp = &((Elf32_External_Phdr *) buf)[i];
		    buf_vaddr_p = (gdb_byte *) &phdrp->p_vaddr;
		    buf_paddr_p = (gdb_byte *) &phdrp->p_paddr;

		    vaddr = extract_unsigned_integer (buf_vaddr_p, 4,
						      byte_order);
		    displacement_vaddr = vaddr - phdr2[i].p_vaddr;

		    paddr = extract_unsigned_integer (buf_paddr_p, 4,
						      byte_order);
		    displacement_paddr = paddr - phdr2[i].p_paddr;

		    if (displacement_vaddr == displacement_paddr)
		      displacement = displacement_vaddr;

		    break;
		  }

	      /* Now compare BUF and BUF2 with optional DISPLACEMENT.  */

	      for (i = 0; i < phdrs_size / sizeof (Elf32_External_Phdr); i++)
		{
		  Elf32_External_Phdr *phdrp;
		  Elf32_External_Phdr *phdr2p;
		  gdb_byte *buf_vaddr_p, *buf_paddr_p;
		  CORE_ADDR vaddr, paddr;
		  asection *plt2_asect;

		  phdrp = &((Elf32_External_Phdr *) buf)[i];
		  buf_vaddr_p = (gdb_byte *) &phdrp->p_vaddr;
		  buf_paddr_p = (gdb_byte *) &phdrp->p_paddr;
		  phdr2p = &((Elf32_External_Phdr *) buf2)[i];

		  /* PT_GNU_STACK is an exception by being never relocated by
		     prelink as its addresses are always zero.  */

		  if (memcmp (phdrp, phdr2p, sizeof (*phdrp)) == 0)
		    continue;

		  /* Check also other adjustment combinations - PR 11786.  */

		  vaddr = extract_unsigned_integer (buf_vaddr_p, 4,
						    byte_order);
		  vaddr -= displacement;
		  store_unsigned_integer (buf_vaddr_p, 4, byte_order, vaddr);

		  paddr = extract_unsigned_integer (buf_paddr_p, 4,
						    byte_order);
		  paddr -= displacement;
		  store_unsigned_integer (buf_paddr_p, 4, byte_order, paddr);

		  if (memcmp (phdrp, phdr2p, sizeof (*phdrp)) == 0)
		    continue;

		  /* prelink can convert .plt SHT_NOBITS to SHT_PROGBITS.  */
		  plt2_asect = bfd_get_section_by_name (exec_bfd, ".plt");
		  if (plt2_asect)
		    {
		      int content2;
		      gdb_byte *buf_filesz_p = (gdb_byte *) &phdrp->p_filesz;
		      CORE_ADDR filesz;

		      content2 = (bfd_get_section_flags (exec_bfd, plt2_asect)
				  & SEC_HAS_CONTENTS) != 0;

		      filesz = extract_unsigned_integer (buf_filesz_p, 4,
							 byte_order);

		      /* PLT2_ASECT is from on-disk file (exec_bfd) while
			 FILESZ is from the in-memory image.  */
		      if (content2)
			filesz += bfd_get_section_size (plt2_asect);
		      else
			filesz -= bfd_get_section_size (plt2_asect);

		      store_unsigned_integer (buf_filesz_p, 4, byte_order,
					      filesz);

		      if (memcmp (phdrp, phdr2p, sizeof (*phdrp)) == 0)
			continue;
		    }

		  ok = 0;
		  break;
		}
	    }
	  else if (arch_size == 64
		   && phdrs_size >= sizeof (Elf64_External_Phdr)
	           && phdrs_size % sizeof (Elf64_External_Phdr) == 0)
	    {
	      Elf_Internal_Ehdr *ehdr2 = elf_tdata (exec_bfd)->elf_header;
	      Elf_Internal_Phdr *phdr2 = elf_tdata (exec_bfd)->phdr;
	      CORE_ADDR displacement = 0;
	      int i;

	      /* DISPLACEMENT could be found more easily by the difference of
		 ehdr2->e_entry.  But we haven't read the ehdr yet, and we
		 already have enough information to compute that displacement
		 with what we've read.  */

	      for (i = 0; i < ehdr2->e_phnum; i++)
		if (phdr2[i].p_type == PT_LOAD)
		  {
		    Elf64_External_Phdr *phdrp;
		    gdb_byte *buf_vaddr_p, *buf_paddr_p;
		    CORE_ADDR vaddr, paddr;
		    CORE_ADDR displacement_vaddr = 0;
		    CORE_ADDR displacement_paddr = 0;

		    phdrp = &((Elf64_External_Phdr *) buf)[i];
		    buf_vaddr_p = (gdb_byte *) &phdrp->p_vaddr;
		    buf_paddr_p = (gdb_byte *) &phdrp->p_paddr;

		    vaddr = extract_unsigned_integer (buf_vaddr_p, 8,
						      byte_order);
		    displacement_vaddr = vaddr - phdr2[i].p_vaddr;

		    paddr = extract_unsigned_integer (buf_paddr_p, 8,
						      byte_order);
		    displacement_paddr = paddr - phdr2[i].p_paddr;

		    if (displacement_vaddr == displacement_paddr)
		      displacement = displacement_vaddr;

		    break;
		  }

	      /* Now compare BUF and BUF2 with optional DISPLACEMENT.  */

	      for (i = 0; i < phdrs_size / sizeof (Elf64_External_Phdr); i++)
		{
		  Elf64_External_Phdr *phdrp;
		  Elf64_External_Phdr *phdr2p;
		  gdb_byte *buf_vaddr_p, *buf_paddr_p;
		  CORE_ADDR vaddr, paddr;
		  asection *plt2_asect;

		  phdrp = &((Elf64_External_Phdr *) buf)[i];
		  buf_vaddr_p = (gdb_byte *) &phdrp->p_vaddr;
		  buf_paddr_p = (gdb_byte *) &phdrp->p_paddr;
		  phdr2p = &((Elf64_External_Phdr *) buf2)[i];

		  /* PT_GNU_STACK is an exception by being never relocated by
		     prelink as its addresses are always zero.  */

		  if (memcmp (phdrp, phdr2p, sizeof (*phdrp)) == 0)
		    continue;

		  /* Check also other adjustment combinations - PR 11786.  */

		  vaddr = extract_unsigned_integer (buf_vaddr_p, 8,
						    byte_order);
		  vaddr -= displacement;
		  store_unsigned_integer (buf_vaddr_p, 8, byte_order, vaddr);

		  paddr = extract_unsigned_integer (buf_paddr_p, 8,
						    byte_order);
		  paddr -= displacement;
		  store_unsigned_integer (buf_paddr_p, 8, byte_order, paddr);

		  if (memcmp (phdrp, phdr2p, sizeof (*phdrp)) == 0)
		    continue;

		  /* prelink can convert .plt SHT_NOBITS to SHT_PROGBITS.  */
		  plt2_asect = bfd_get_section_by_name (exec_bfd, ".plt");
		  if (plt2_asect)
		    {
		      int content2;
		      gdb_byte *buf_filesz_p = (gdb_byte *) &phdrp->p_filesz;
		      CORE_ADDR filesz;

		      content2 = (bfd_get_section_flags (exec_bfd, plt2_asect)
				  & SEC_HAS_CONTENTS) != 0;

		      filesz = extract_unsigned_integer (buf_filesz_p, 8,
							 byte_order);

		      /* PLT2_ASECT is from on-disk file (exec_bfd) while
			 FILESZ is from the in-memory image.  */
		      if (content2)
			filesz += bfd_get_section_size (plt2_asect);
		      else
			filesz -= bfd_get_section_size (plt2_asect);

		      store_unsigned_integer (buf_filesz_p, 8, byte_order,
					      filesz);

		      if (memcmp (phdrp, phdr2p, sizeof (*phdrp)) == 0)
			continue;
		    }

		  ok = 0;
		  break;
		}
	    }
	  else
	    ok = 0;
	}

      xfree (buf);
      xfree (buf2);

      if (!ok)
	return 0;
    }

  if (info_verbose)
    {
      /* It can be printed repeatedly as there is no easy way to check
	 the executable symbols/file has been already relocated to
	 displacement.  */

      printf_unfiltered (_("Using PIE (Position Independent Executable) "
			   "displacement %s for \"%s\".\n"),
			 paddress (target_gdbarch (), displacement),
			 bfd_get_filename (exec_bfd));
    }

  *displacementp = displacement;
  return 1;
}

/* Relocate the main executable.  This function should be called upon
   stopping the inferior process at the entry point to the program.
   The entry point from BFD is compared to the AT_ENTRY of AUXV and if they are
   different, the main executable is relocated by the proper amount.  */

static void
svr4_relocate_main_executable (void)
{
  CORE_ADDR displacement;

  /* If we are re-running this executable, SYMFILE_OBJFILE->SECTION_OFFSETS
     probably contains the offsets computed using the PIE displacement
     from the previous run, which of course are irrelevant for this run.
     So we need to determine the new PIE displacement and recompute the
     section offsets accordingly, even if SYMFILE_OBJFILE->SECTION_OFFSETS
     already contains pre-computed offsets.

     If we cannot compute the PIE displacement, either:

       - The executable is not PIE.

       - SYMFILE_OBJFILE does not match the executable started in the target.
	 This can happen for main executable symbols loaded at the host while
	 `ld.so --ld-args main-executable' is loaded in the target.

     Then we leave the section offsets untouched and use them as is for
     this run.  Either:

       - These section offsets were properly reset earlier, and thus
	 already contain the correct values.  This can happen for instance
	 when reconnecting via the remote protocol to a target that supports
	 the `qOffsets' packet.

       - The section offsets were not reset earlier, and the best we can
	 hope is that the old offsets are still applicable to the new run.  */

  if (! svr4_exec_displacement (&displacement))
    return;

  /* Even DISPLACEMENT 0 is a valid new difference of in-memory vs. in-file
     addresses.  */

  if (symfile_objfile)
    {
      struct section_offsets *new_offsets;
      int i;

      new_offsets = alloca (symfile_objfile->num_sections
			    * sizeof (*new_offsets));

      for (i = 0; i < symfile_objfile->num_sections; i++)
	new_offsets->offsets[i] = displacement;

      objfile_relocate (symfile_objfile, new_offsets);
    }
  else if (exec_bfd)
    {
      asection *asect;

      for (asect = exec_bfd->sections; asect != NULL; asect = asect->next)
	exec_set_section_address (bfd_get_filename (exec_bfd), asect->index,
				  (bfd_section_vma (exec_bfd, asect)
				   + displacement));
    }
}

/* Implement the "create_inferior_hook" target_solib_ops method.

   For SVR4 executables, this first instruction is either the first
   instruction in the dynamic linker (for dynamically linked
   executables) or the instruction at "start" for statically linked
   executables.  For dynamically linked executables, the system
   first exec's /lib/libc.so.N, which contains the dynamic linker,
   and starts it running.  The dynamic linker maps in any needed
   shared libraries, maps in the actual user executable, and then
   jumps to "start" in the user executable.

   We can arrange to cooperate with the dynamic linker to discover the
   names of shared libraries that are dynamically linked, and the base
   addresses to which they are linked.

   This function is responsible for discovering those names and
   addresses, and saving sufficient information about them to allow
   their symbols to be read at a later time.  */

static void
svr4_solib_create_inferior_hook (int from_tty)
{
  struct svr4_info *info;

  info = get_svr4_info ();

  /* Relocate the main executable if necessary.  */
  svr4_relocate_main_executable ();

  /* No point setting a breakpoint in the dynamic linker if we can't
     hit it (e.g., a core file, or a trace file).  */
  if (!target_has_execution)
    return;

  if (!svr4_have_link_map_offsets ())
    return;

  if (!enable_break (info, from_tty))
    return;
}

static void
svr4_clear_solib (void)
{
  struct svr4_info *info;

  info = get_svr4_info ();
  info->debug_base = 0;
  info->debug_loader_offset_p = 0;
  info->debug_loader_offset = 0;
  xfree (info->debug_loader_name);
  info->debug_loader_name = NULL;
}

/* Clear any bits of ADDR that wouldn't fit in a target-format
   data pointer.  "Data pointer" here refers to whatever sort of
   address the dynamic linker uses to manage its sections.  At the
   moment, we don't support shared libraries on any processors where
   code and data pointers are different sizes.

   This isn't really the right solution.  What we really need here is
   a way to do arithmetic on CORE_ADDR values that respects the
   natural pointer/address correspondence.  (For example, on the MIPS,
   converting a 32-bit pointer to a 64-bit CORE_ADDR requires you to
   sign-extend the value.  There, simply truncating the bits above
   gdbarch_ptr_bit, as we do below, is no good.)  This should probably
   be a new gdbarch method or something.  */
static CORE_ADDR
svr4_truncate_ptr (CORE_ADDR addr)
{
  if (gdbarch_ptr_bit (target_gdbarch ()) == sizeof (CORE_ADDR) * 8)
    /* We don't need to truncate anything, and the bit twiddling below
       will fail due to overflow problems.  */
    return addr;
  else
    return addr & (((CORE_ADDR) 1 << gdbarch_ptr_bit (target_gdbarch ())) - 1);
}


static void
svr4_relocate_section_addresses (struct so_list *so,
                                 struct target_section *sec)
{
  sec->addr    = svr4_truncate_ptr (sec->addr    + lm_addr_check (so,
								  sec->bfd));
  sec->endaddr = svr4_truncate_ptr (sec->endaddr + lm_addr_check (so,
								  sec->bfd));
}


/* Architecture-specific operations.  */

/* Per-architecture data key.  */
static struct gdbarch_data *solib_svr4_data;

struct solib_svr4_ops
{
  /* Return a description of the layout of `struct link_map'.  */
  struct link_map_offsets *(*fetch_link_map_offsets)(void);
};

/* Return a default for the architecture-specific operations.  */

static void *
solib_svr4_init (struct obstack *obstack)
{
  struct solib_svr4_ops *ops;

  ops = OBSTACK_ZALLOC (obstack, struct solib_svr4_ops);
  ops->fetch_link_map_offsets = NULL;
  return ops;
}

/* Set the architecture-specific `struct link_map_offsets' fetcher for
   GDBARCH to FLMO.  Also, install SVR4 solib_ops into GDBARCH.  */

void
set_solib_svr4_fetch_link_map_offsets (struct gdbarch *gdbarch,
                                       struct link_map_offsets *(*flmo) (void))
{
  struct solib_svr4_ops *ops = gdbarch_data (gdbarch, solib_svr4_data);

  ops->fetch_link_map_offsets = flmo;

  set_solib_ops (gdbarch, &svr4_so_ops);
}

/* Fetch a link_map_offsets structure using the architecture-specific
   `struct link_map_offsets' fetcher.  */

static struct link_map_offsets *
svr4_fetch_link_map_offsets (void)
{
  struct solib_svr4_ops *ops = gdbarch_data (target_gdbarch (), solib_svr4_data);

  gdb_assert (ops->fetch_link_map_offsets);
  return ops->fetch_link_map_offsets ();
}

/* Return 1 if a link map offset fetcher has been defined, 0 otherwise.  */

static int
svr4_have_link_map_offsets (void)
{
  struct solib_svr4_ops *ops = gdbarch_data (target_gdbarch (), solib_svr4_data);

  return (ops->fetch_link_map_offsets != NULL);
}


/* Most OS'es that have SVR4-style ELF dynamic libraries define a
   `struct r_debug' and a `struct link_map' that are binary compatible
   with the origional SVR4 implementation.  */

/* Fetch (and possibly build) an appropriate `struct link_map_offsets'
   for an ILP32 SVR4 system.  */

struct link_map_offsets *
svr4_ilp32_fetch_link_map_offsets (void)
{
  static struct link_map_offsets lmo;
  static struct link_map_offsets *lmp = NULL;

  if (lmp == NULL)
    {
      lmp = &lmo;

      lmo.r_version_offset = 0;
      lmo.r_version_size = 4;
      lmo.r_map_offset = 4;
      lmo.r_brk_offset = 8;
      lmo.r_ldsomap_offset = 20;

      /* Everything we need is in the first 20 bytes.  */
      lmo.link_map_size = 20;
      lmo.l_addr_offset = 0;
      lmo.l_name_offset = 4;
      lmo.l_ld_offset = 8;
      lmo.l_next_offset = 12;
      lmo.l_prev_offset = 16;
    }

  return lmp;
}

/* Fetch (and possibly build) an appropriate `struct link_map_offsets'
   for an LP64 SVR4 system.  */

struct link_map_offsets *
svr4_lp64_fetch_link_map_offsets (void)
{
  static struct link_map_offsets lmo;
  static struct link_map_offsets *lmp = NULL;

  if (lmp == NULL)
    {
      lmp = &lmo;

      lmo.r_version_offset = 0;
      lmo.r_version_size = 4;
      lmo.r_map_offset = 8;
      lmo.r_brk_offset = 16;
      lmo.r_ldsomap_offset = 40;

      /* Everything we need is in the first 40 bytes.  */
      lmo.link_map_size = 40;
      lmo.l_addr_offset = 0;
      lmo.l_name_offset = 8;
      lmo.l_ld_offset = 16;
      lmo.l_next_offset = 24;
      lmo.l_prev_offset = 32;
    }

  return lmp;
}


struct target_so_ops svr4_so_ops;

/* Lookup global symbol for ELF DSOs linked with -Bsymbolic.  Those DSOs have a
   different rule for symbol lookup.  The lookup begins here in the DSO, not in
   the main executable.  */

static struct symbol *
elf_lookup_lib_symbol (const struct objfile *objfile,
		       const char *name,
		       const domain_enum domain)
{
  bfd *abfd;

  if (objfile == symfile_objfile)
    abfd = exec_bfd;
  else
    {
      /* OBJFILE should have been passed as the non-debug one.  */
      gdb_assert (objfile->separate_debug_objfile_backlink == NULL);

      abfd = objfile->obfd;
    }

  if (abfd == NULL || scan_dyntag (DT_SYMBOLIC, abfd, NULL) != 1)
    return NULL;

  return lookup_global_symbol_from_objfile (objfile, name, domain);
}

extern initialize_file_ftype _initialize_svr4_solib; /* -Wmissing-prototypes */

void
_initialize_svr4_solib (void)
{
  solib_svr4_data = gdbarch_data_register_pre_init (solib_svr4_init);
  solib_svr4_pspace_data
    = register_program_space_data_with_cleanup (NULL, svr4_pspace_data_cleanup);

  svr4_so_ops.relocate_section_addresses = svr4_relocate_section_addresses;
  svr4_so_ops.free_so = svr4_free_so;
  svr4_so_ops.clear_solib = svr4_clear_solib;
  svr4_so_ops.solib_create_inferior_hook = svr4_solib_create_inferior_hook;
  svr4_so_ops.special_symbol_handling = svr4_special_symbol_handling;
  svr4_so_ops.current_sos = svr4_current_sos;
  svr4_so_ops.open_symbol_file_object = open_symbol_file_object;
  svr4_so_ops.in_dynsym_resolve_code = svr4_in_dynsym_resolve_code;
  svr4_so_ops.bfd_open = solib_bfd_open;
  svr4_so_ops.lookup_lib_global_symbol = elf_lookup_lib_symbol;
  svr4_so_ops.same = svr4_same;
  svr4_so_ops.keep_data_in_core = svr4_keep_data_in_core;
}
