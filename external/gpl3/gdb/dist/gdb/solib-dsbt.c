/* Handle TIC6X (DSBT) shared libraries for GDB, the GNU Debugger.
   Copyright (C) 2010-2013 Free Software Foundation, Inc.

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
#include "gdb_string.h"
#include "inferior.h"
#include "gdbcore.h"
#include "solib.h"
#include "solist.h"
#include "objfiles.h"
#include "symtab.h"
#include "language.h"
#include "command.h"
#include "gdbcmd.h"
#include "elf-bfd.h"
#include "exceptions.h"
#include "gdb_bfd.h"

#define GOT_MODULE_OFFSET 4

/* Flag which indicates whether internal debug messages should be printed.  */
static unsigned int solib_dsbt_debug = 0;

/* TIC6X pointers are four bytes wide.  */
enum { TIC6X_PTR_SIZE = 4 };

/* Representation of loadmap and related structs for the TIC6X DSBT.  */

/* External versions; the size and alignment of the fields should be
   the same as those on the target.  When loaded, the placement of
   the bits in each field will be the same as on the target.  */
typedef gdb_byte ext_Elf32_Half[2];
typedef gdb_byte ext_Elf32_Addr[4];
typedef gdb_byte ext_Elf32_Word[4];

struct ext_elf32_dsbt_loadseg
{
  /* Core address to which the segment is mapped.  */
  ext_Elf32_Addr addr;
  /* VMA recorded in the program header.  */
  ext_Elf32_Addr p_vaddr;
  /* Size of this segment in memory.  */
  ext_Elf32_Word p_memsz;
};

struct ext_elf32_dsbt_loadmap {
  /* Protocol version number, must be zero.  */
  ext_Elf32_Word version;
  /* A pointer to the DSBT table; the DSBT size and the index of this
     module.  */
  ext_Elf32_Word dsbt_table_ptr;
  ext_Elf32_Word dsbt_size;
  ext_Elf32_Word dsbt_index;
  /* Number of segments in this map.  */
  ext_Elf32_Word nsegs;
  /* The actual memory map.  */
  struct ext_elf32_dsbt_loadseg segs[1 /* nsegs, actually */];
};

/* Internal versions; the types are GDB types and the data in each
   of the fields is (or will be) decoded from the external struct
   for ease of consumption.  */
struct int_elf32_dsbt_loadseg
{
  /* Core address to which the segment is mapped.  */
  CORE_ADDR addr;
  /* VMA recorded in the program header.  */
  CORE_ADDR p_vaddr;
  /* Size of this segment in memory.  */
  long p_memsz;
};

struct int_elf32_dsbt_loadmap
{
  /* Protocol version number, must be zero.  */
  int version;
  CORE_ADDR dsbt_table_ptr;
  /* A pointer to the DSBT table; the DSBT size and the index of this
     module.  */
  int dsbt_size, dsbt_index;
  /* Number of segments in this map.  */
  int nsegs;
  /* The actual memory map.  */
  struct int_elf32_dsbt_loadseg segs[1 /* nsegs, actually */];
};

/* External link_map and elf32_dsbt_loadaddr struct definitions.  */

typedef gdb_byte ext_ptr[4];

struct ext_elf32_dsbt_loadaddr
{
  ext_ptr map;			/* struct elf32_dsbt_loadmap *map; */
};

struct ext_link_map
{
  struct ext_elf32_dsbt_loadaddr l_addr;

  /* Absolute file name object was found in.  */
  ext_ptr l_name;		/* char *l_name; */

  /* Dynamic section of the shared object.  */
  ext_ptr l_ld;			/* ElfW(Dyn) *l_ld; */

  /* Chain of loaded objects.  */
  ext_ptr l_next, l_prev;	/* struct link_map *l_next, *l_prev; */
};

/* Link map info to include in an allocated so_list entry */

struct lm_info
{
  /* The loadmap, digested into an easier to use form.  */
  struct int_elf32_dsbt_loadmap *map;
};

/* Per pspace dsbt specific data.  */

struct dsbt_info
{
  /* The load map, got value, etc. are not available from the chain
     of loaded shared objects.  ``main_executable_lm_info'' provides
     a way to get at this information so that it doesn't need to be
     frequently recomputed.  Initialized by dsbt_relocate_main_executable.  */
  struct lm_info *main_executable_lm_info;

  /* Load maps for the main executable and the interpreter.  These are obtained
     from ptrace.  They are the starting point for getting into the program,
     and are required to find the solib list with the individual load maps for
     each module.  */
  struct int_elf32_dsbt_loadmap *exec_loadmap;
  struct int_elf32_dsbt_loadmap *interp_loadmap;

  /* Cached value for lm_base, below.  */
  CORE_ADDR lm_base_cache;

  /* Link map address for main module.  */
  CORE_ADDR main_lm_addr;

  int enable_break2_done;

  CORE_ADDR interp_text_sect_low;
  CORE_ADDR interp_text_sect_high;
  CORE_ADDR interp_plt_sect_low;
  CORE_ADDR interp_plt_sect_high;
};

/* Per-program-space data key.  */
static const struct program_space_data *solib_dsbt_pspace_data;

static void
dsbt_pspace_data_cleanup (struct program_space *pspace, void *arg)
{
  struct dsbt_info *info;

  info = program_space_data (pspace, solib_dsbt_pspace_data);
  xfree (info);
}

/* Get the current dsbt data.  If none is found yet, add it now.  This
   function always returns a valid object.  */

static struct dsbt_info *
get_dsbt_info (void)
{
  struct dsbt_info *info;

  info = program_space_data (current_program_space, solib_dsbt_pspace_data);
  if (info != NULL)
    return info;

  info = XZALLOC (struct dsbt_info);
  set_program_space_data (current_program_space, solib_dsbt_pspace_data, info);

  info->enable_break2_done = 0;
  info->lm_base_cache = 0;
  info->main_lm_addr = 0;

  return info;
}


static void
dsbt_print_loadmap (struct int_elf32_dsbt_loadmap *map)
{
  int i;

  if (map == NULL)
    printf_filtered ("(null)\n");
  else if (map->version != 0)
    printf_filtered (_("Unsupported map version: %d\n"), map->version);
  else
    {
      printf_filtered ("version %d\n", map->version);

      for (i = 0; i < map->nsegs; i++)
	printf_filtered ("%s:%s -> %s:%s\n",
			 print_core_address (target_gdbarch (),
					     map->segs[i].p_vaddr),
			 print_core_address (target_gdbarch (),
					     map->segs[i].p_vaddr
					     + map->segs[i].p_memsz),
			 print_core_address (target_gdbarch (), map->segs[i].addr),
			 print_core_address (target_gdbarch (), map->segs[i].addr
					     + map->segs[i].p_memsz));
    }
}

/* Decode int_elf32_dsbt_loadmap from BUF.  */

static struct int_elf32_dsbt_loadmap *
decode_loadmap (gdb_byte *buf)
{
  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());
  struct ext_elf32_dsbt_loadmap *ext_ldmbuf;
  struct int_elf32_dsbt_loadmap *int_ldmbuf;

  int version, seg, nsegs;
  int int_ldmbuf_size;

  ext_ldmbuf = (struct ext_elf32_dsbt_loadmap *) buf;

  /* Extract the version.  */
  version = extract_unsigned_integer (ext_ldmbuf->version,
				      sizeof ext_ldmbuf->version,
				      byte_order);
  if (version != 0)
    {
      /* We only handle version 0.  */
      return NULL;
    }

  /* Extract the number of segments.  */
  nsegs = extract_unsigned_integer (ext_ldmbuf->nsegs,
				    sizeof ext_ldmbuf->nsegs,
				    byte_order);

  if (nsegs <= 0)
    return NULL;

  /* Allocate space into which to put information extract from the
     external loadsegs.  I.e, allocate the internal loadsegs.  */
  int_ldmbuf_size = (sizeof (struct int_elf32_dsbt_loadmap)
		     + (nsegs - 1) * sizeof (struct int_elf32_dsbt_loadseg));
  int_ldmbuf = xmalloc (int_ldmbuf_size);

  /* Place extracted information in internal structs.  */
  int_ldmbuf->version = version;
  int_ldmbuf->nsegs = nsegs;
  for (seg = 0; seg < nsegs; seg++)
    {
      int_ldmbuf->segs[seg].addr
	= extract_unsigned_integer (ext_ldmbuf->segs[seg].addr,
				    sizeof (ext_ldmbuf->segs[seg].addr),
				    byte_order);
      int_ldmbuf->segs[seg].p_vaddr
	= extract_unsigned_integer (ext_ldmbuf->segs[seg].p_vaddr,
				    sizeof (ext_ldmbuf->segs[seg].p_vaddr),
				    byte_order);
      int_ldmbuf->segs[seg].p_memsz
	= extract_unsigned_integer (ext_ldmbuf->segs[seg].p_memsz,
				    sizeof (ext_ldmbuf->segs[seg].p_memsz),
				    byte_order);
    }

  xfree (ext_ldmbuf);
  return int_ldmbuf;
}


static struct dsbt_info *get_dsbt_info (void);

/* Interrogate the Linux kernel to find out where the program was loaded.
   There are two load maps; one for the executable and one for the
   interpreter (only in the case of a dynamically linked executable).  */

static void
dsbt_get_initial_loadmaps (void)
{
  gdb_byte *buf;
  struct dsbt_info *info = get_dsbt_info ();

  if (0 >= target_read_alloc (&current_target, TARGET_OBJECT_FDPIC,
			      "exec", (gdb_byte**) &buf))
    {
      info->exec_loadmap = NULL;
      error (_("Error reading DSBT exec loadmap"));
    }
  info->exec_loadmap = decode_loadmap (buf);
  if (solib_dsbt_debug)
    dsbt_print_loadmap (info->exec_loadmap);

  if (0 >= target_read_alloc (&current_target, TARGET_OBJECT_FDPIC,
			      "interp", (gdb_byte**)&buf))
    {
      info->interp_loadmap = NULL;
      error (_("Error reading DSBT interp loadmap"));
    }
  info->interp_loadmap = decode_loadmap (buf);
  if (solib_dsbt_debug)
    dsbt_print_loadmap (info->interp_loadmap);
}

/* Given address LDMADDR, fetch and decode the loadmap at that address.
   Return NULL if there is a problem reading the target memory or if
   there doesn't appear to be a loadmap at the given address.  The
   allocated space (representing the loadmap) returned by this
   function may be freed via a single call to xfree.  */

static struct int_elf32_dsbt_loadmap *
fetch_loadmap (CORE_ADDR ldmaddr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());
  struct ext_elf32_dsbt_loadmap ext_ldmbuf_partial;
  struct ext_elf32_dsbt_loadmap *ext_ldmbuf;
  struct int_elf32_dsbt_loadmap *int_ldmbuf;
  int ext_ldmbuf_size, int_ldmbuf_size;
  int version, seg, nsegs;

  /* Fetch initial portion of the loadmap.  */
  if (target_read_memory (ldmaddr, (gdb_byte *) &ext_ldmbuf_partial,
                          sizeof ext_ldmbuf_partial))
    {
      /* Problem reading the target's memory.  */
      return NULL;
    }

  /* Extract the version.  */
  version = extract_unsigned_integer (ext_ldmbuf_partial.version,
                                      sizeof ext_ldmbuf_partial.version,
				      byte_order);
  if (version != 0)
    {
      /* We only handle version 0.  */
      return NULL;
    }

  /* Extract the number of segments.  */
  nsegs = extract_unsigned_integer (ext_ldmbuf_partial.nsegs,
				    sizeof ext_ldmbuf_partial.nsegs,
				    byte_order);

  if (nsegs <= 0)
    return NULL;

  /* Allocate space for the complete (external) loadmap.  */
  ext_ldmbuf_size = sizeof (struct ext_elf32_dsbt_loadmap)
    + (nsegs - 1) * sizeof (struct ext_elf32_dsbt_loadseg);
  ext_ldmbuf = xmalloc (ext_ldmbuf_size);

  /* Copy over the portion of the loadmap that's already been read.  */
  memcpy (ext_ldmbuf, &ext_ldmbuf_partial, sizeof ext_ldmbuf_partial);

  /* Read the rest of the loadmap from the target.  */
  if (target_read_memory (ldmaddr + sizeof ext_ldmbuf_partial,
			  (gdb_byte *) ext_ldmbuf + sizeof ext_ldmbuf_partial,
			  ext_ldmbuf_size - sizeof ext_ldmbuf_partial))
    {
      /* Couldn't read rest of the loadmap.  */
      xfree (ext_ldmbuf);
      return NULL;
    }

  /* Allocate space into which to put information extract from the
     external loadsegs.  I.e, allocate the internal loadsegs.  */
  int_ldmbuf_size = sizeof (struct int_elf32_dsbt_loadmap)
    + (nsegs - 1) * sizeof (struct int_elf32_dsbt_loadseg);
  int_ldmbuf = xmalloc (int_ldmbuf_size);

  /* Place extracted information in internal structs.  */
  int_ldmbuf->version = version;
  int_ldmbuf->nsegs = nsegs;
  for (seg = 0; seg < nsegs; seg++)
    {
      int_ldmbuf->segs[seg].addr
	= extract_unsigned_integer (ext_ldmbuf->segs[seg].addr,
				    sizeof (ext_ldmbuf->segs[seg].addr),
				    byte_order);
      int_ldmbuf->segs[seg].p_vaddr
	= extract_unsigned_integer (ext_ldmbuf->segs[seg].p_vaddr,
				    sizeof (ext_ldmbuf->segs[seg].p_vaddr),
				    byte_order);
      int_ldmbuf->segs[seg].p_memsz
	= extract_unsigned_integer (ext_ldmbuf->segs[seg].p_memsz,
				    sizeof (ext_ldmbuf->segs[seg].p_memsz),
				    byte_order);
    }

  xfree (ext_ldmbuf);
  return int_ldmbuf;
}

static void dsbt_relocate_main_executable (void);
static int enable_break2 (void);

/* Scan for DYNTAG in .dynamic section of ABFD. If DYNTAG is found 1 is
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

/* If no open symbol file, attempt to locate and open the main symbol
   file.

   If FROM_TTYP dereferences to a non-zero integer, allow messages to
   be printed.  This parameter is a pointer rather than an int because
   open_symbol_file_object is called via catch_errors and
   catch_errors requires a pointer argument. */

static int
open_symbol_file_object (void *from_ttyp)
{
  /* Unimplemented.  */
  return 0;
}

/* Given a loadmap and an address, return the displacement needed
   to relocate the address.  */

static CORE_ADDR
displacement_from_map (struct int_elf32_dsbt_loadmap *map,
                       CORE_ADDR addr)
{
  int seg;

  for (seg = 0; seg < map->nsegs; seg++)
    if (map->segs[seg].p_vaddr <= addr
	&& addr < map->segs[seg].p_vaddr + map->segs[seg].p_memsz)
      return map->segs[seg].addr - map->segs[seg].p_vaddr;

  return 0;
}

/* Return the address from which the link map chain may be found.  On
   DSBT, a pointer to the start of the link map will be located at the
   word found at base of GOT + GOT_MODULE_OFFSET.

   The base of GOT may be found in a number of ways.  Assuming that the
   main executable has already been relocated,
   1 The easiest way to find this value is to look up the address of
   _GLOBAL_OFFSET_TABLE_.
   2 The other way is to look for tag DT_PLTGOT, which contains the virtual
   address of Global Offset Table.  .*/

static CORE_ADDR
lm_base (void)
{
  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());
  struct minimal_symbol *got_sym;
  CORE_ADDR addr;
  gdb_byte buf[TIC6X_PTR_SIZE];
  struct dsbt_info *info = get_dsbt_info ();

  /* One of our assumptions is that the main executable has been relocated.
     Bail out if this has not happened.  (Note that post_create_inferior
     in infcmd.c will call solib_add prior to solib_create_inferior_hook.
     If we allow this to happen, lm_base_cache will be initialized with
     a bogus value.  */
  if (info->main_executable_lm_info == 0)
    return 0;

  /* If we already have a cached value, return it.  */
  if (info->lm_base_cache)
    return info->lm_base_cache;

  got_sym = lookup_minimal_symbol ("_GLOBAL_OFFSET_TABLE_", NULL,
				   symfile_objfile);

  if (got_sym != 0)
    {
      addr = SYMBOL_VALUE_ADDRESS (got_sym);
      if (solib_dsbt_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "lm_base: get addr %x by _GLOBAL_OFFSET_TABLE_.\n",
			    (unsigned int) addr);
    }
  else if (scan_dyntag (DT_PLTGOT, exec_bfd, &addr))
    {
      struct int_elf32_dsbt_loadmap *ldm;

      dsbt_get_initial_loadmaps ();
      ldm = info->exec_loadmap;
      addr += displacement_from_map (ldm, addr);
      if (solib_dsbt_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "lm_base: get addr %x by DT_PLTGOT.\n",
			    (unsigned int) addr);
    }
  else
    {
      if (solib_dsbt_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "lm_base: _GLOBAL_OFFSET_TABLE_ not found.\n");
      return 0;
    }
  addr += GOT_MODULE_OFFSET;

  if (solib_dsbt_debug)
    fprintf_unfiltered (gdb_stdlog,
			"lm_base: _GLOBAL_OFFSET_TABLE_ + %d = %s\n",
			GOT_MODULE_OFFSET, hex_string_custom (addr, 8));

  if (target_read_memory (addr, buf, sizeof buf) != 0)
    return 0;
  info->lm_base_cache = extract_unsigned_integer (buf, sizeof buf, byte_order);

  if (solib_dsbt_debug)
    fprintf_unfiltered (gdb_stdlog,
			"lm_base: lm_base_cache = %s\n",
			hex_string_custom (info->lm_base_cache, 8));

  return info->lm_base_cache;
}


/* Build a list of `struct so_list' objects describing the shared
   objects currently loaded in the inferior.  This list does not
   include an entry for the main executable file.

   Note that we only gather information directly available from the
   inferior --- we don't examine any of the shared library files
   themselves.  The declaration of `struct so_list' says which fields
   we provide values for.  */

static struct so_list *
dsbt_current_sos (void)
{
  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());
  CORE_ADDR lm_addr;
  struct so_list *sos_head = NULL;
  struct so_list **sos_next_ptr = &sos_head;
  struct dsbt_info *info = get_dsbt_info ();

  /* Make sure that the main executable has been relocated.  This is
     required in order to find the address of the global offset table,
     which in turn is used to find the link map info.  (See lm_base
     for details.)

     Note that the relocation of the main executable is also performed
     by SOLIB_CREATE_INFERIOR_HOOK, however, in the case of core
     files, this hook is called too late in order to be of benefit to
     SOLIB_ADD.  SOLIB_ADD eventually calls this function,
     dsbt_current_sos, and also precedes the call to
     SOLIB_CREATE_INFERIOR_HOOK.   (See post_create_inferior in
     infcmd.c.)  */
  if (info->main_executable_lm_info == 0 && core_bfd != NULL)
    dsbt_relocate_main_executable ();

  /* Locate the address of the first link map struct.  */
  lm_addr = lm_base ();

  /* We have at least one link map entry.  Fetch the the lot of them,
     building the solist chain.  */
  while (lm_addr)
    {
      struct ext_link_map lm_buf;
      ext_Elf32_Word indexword;
      CORE_ADDR map_addr;
      int dsbt_index;
      int ret;

      if (solib_dsbt_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "current_sos: reading link_map entry at %s\n",
			    hex_string_custom (lm_addr, 8));

      ret = target_read_memory (lm_addr, (gdb_byte *) &lm_buf, sizeof (lm_buf));
      if (ret)
	{
	  warning (_("dsbt_current_sos: Unable to read link map entry."
		     "  Shared object chain may be incomplete."));
	  break;
	}

      /* Fetch the load map address.  */
      map_addr = extract_unsigned_integer (lm_buf.l_addr.map,
					   sizeof lm_buf.l_addr.map,
					   byte_order);

      ret = target_read_memory (map_addr + 12, (gdb_byte *) &indexword,
				sizeof indexword);
      if (ret)
	{
	  warning (_("dsbt_current_sos: Unable to read dsbt index."
		     "  Shared object chain may be incomplete."));
	  break;
	}
      dsbt_index = extract_unsigned_integer (indexword, sizeof indexword,
					     byte_order);

      /* If the DSBT index is zero, then we're looking at the entry
	 for the main executable.  By convention, we don't include
	 this in the list of shared objects.  */
      if (dsbt_index != 0)
	{
	  int errcode;
	  char *name_buf;
	  struct int_elf32_dsbt_loadmap *loadmap;
	  struct so_list *sop;
	  CORE_ADDR addr;

	  loadmap = fetch_loadmap (map_addr);
	  if (loadmap == NULL)
	    {
	      warning (_("dsbt_current_sos: Unable to fetch load map."
			 "  Shared object chain may be incomplete."));
	      break;
	    }

	  sop = xcalloc (1, sizeof (struct so_list));
	  sop->lm_info = xcalloc (1, sizeof (struct lm_info));
	  sop->lm_info->map = loadmap;
	  /* Fetch the name.  */
	  addr = extract_unsigned_integer (lm_buf.l_name,
					   sizeof (lm_buf.l_name),
					   byte_order);
	  target_read_string (addr, &name_buf, SO_NAME_MAX_PATH_SIZE - 1,
			      &errcode);

	  if (errcode != 0)
	    warning (_("Can't read pathname for link map entry: %s."),
		     safe_strerror (errcode));
	  else
	    {
	      if (solib_dsbt_debug)
		fprintf_unfiltered (gdb_stdlog, "current_sos: name = %s\n",
				    name_buf);

	      strncpy (sop->so_name, name_buf, SO_NAME_MAX_PATH_SIZE - 1);
	      sop->so_name[SO_NAME_MAX_PATH_SIZE - 1] = '\0';
	      xfree (name_buf);
	      strcpy (sop->so_original_name, sop->so_name);
	    }

	  *sos_next_ptr = sop;
	  sos_next_ptr = &sop->next;
	}
      else
	{
	  info->main_lm_addr = lm_addr;
	}

      lm_addr = extract_unsigned_integer (lm_buf.l_next,
					  sizeof (lm_buf.l_next), byte_order);
    }

  enable_break2 ();

  return sos_head;
}

/* Return 1 if PC lies in the dynamic symbol resolution code of the
   run time loader.  */

static int
dsbt_in_dynsym_resolve_code (CORE_ADDR pc)
{
  struct dsbt_info *info = get_dsbt_info ();

  return ((pc >= info->interp_text_sect_low && pc < info->interp_text_sect_high)
	  || (pc >= info->interp_plt_sect_low && pc < info->interp_plt_sect_high)
	  || in_plt_section (pc, NULL));
}

/* Print a warning about being unable to set the dynamic linker
   breakpoint.  */

static void
enable_break_failure_warning (void)
{
  warning (_("Unable to find dynamic linker breakpoint function.\n"
           "GDB will be unable to debug shared library initializers\n"
	   "and track explicitly loaded dynamic code."));
}

/* Helper function for gdb_bfd_lookup_symbol.  */

static int
cmp_name (asymbol *sym, void *data)
{
  return (strcmp (sym->name, (const char *) data) == 0);
}

/* The dynamic linkers has, as part of its debugger interface, support
   for arranging for the inferior to hit a breakpoint after mapping in
   the shared libraries.  This function enables that breakpoint.

   On the TIC6X, using the shared library (DSBT), the symbol
   _dl_debug_addr points to the r_debug struct which contains
   a field called r_brk.  r_brk is the address of the function
   descriptor upon which a breakpoint must be placed.  Being a
   function descriptor, we must extract the entry point in order
   to set the breakpoint.

   Our strategy will be to get the .interp section from the
   executable.  This section will provide us with the name of the
   interpreter.  We'll open the interpreter and then look up
   the address of _dl_debug_addr.  We then relocate this address
   using the interpreter's loadmap.  Once the relocated address
   is known, we fetch the value (address) corresponding to r_brk
   and then use that value to fetch the entry point of the function
   we're interested in.  */

static int
enable_break2 (void)
{
  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());
  int success = 0;
  char **bkpt_namep;
  asection *interp_sect;
  struct dsbt_info *info = get_dsbt_info ();

  if (exec_bfd == NULL)
    return 0;

  if (!target_has_execution)
    return 0;

  if (info->enable_break2_done)
    return 1;

  info->interp_text_sect_low = 0;
  info->interp_text_sect_high = 0;
  info->interp_plt_sect_low = 0;
  info->interp_plt_sect_high = 0;

  /* Find the .interp section; if not found, warn the user and drop
     into the old breakpoint at symbol code.  */
  interp_sect = bfd_get_section_by_name (exec_bfd, ".interp");
  if (interp_sect)
    {
      unsigned int interp_sect_size;
      gdb_byte *buf;
      bfd *tmp_bfd = NULL;
      CORE_ADDR addr;
      gdb_byte addr_buf[TIC6X_PTR_SIZE];
      struct int_elf32_dsbt_loadmap *ldm;
      volatile struct gdb_exception ex;

      /* Read the contents of the .interp section into a local buffer;
         the contents specify the dynamic linker this program uses.  */
      interp_sect_size = bfd_section_size (exec_bfd, interp_sect);
      buf = alloca (interp_sect_size);
      bfd_get_section_contents (exec_bfd, interp_sect,
				buf, 0, interp_sect_size);

      /* Now we need to figure out where the dynamic linker was
         loaded so that we can load its symbols and place a breakpoint
         in the dynamic linker itself.  */

      TRY_CATCH (ex, RETURN_MASK_ALL)
        {
          tmp_bfd = solib_bfd_open (buf);
        }
      if (tmp_bfd == NULL)
	{
	  enable_break_failure_warning ();
	  return 0;
	}

      dsbt_get_initial_loadmaps ();
      ldm = info->interp_loadmap;

      /* Record the relocated start and end address of the dynamic linker
         text and plt section for dsbt_in_dynsym_resolve_code.  */
      interp_sect = bfd_get_section_by_name (tmp_bfd, ".text");
      if (interp_sect)
	{
	  info->interp_text_sect_low
	    = bfd_section_vma (tmp_bfd, interp_sect);
	  info->interp_text_sect_low
	    += displacement_from_map (ldm, info->interp_text_sect_low);
	  info->interp_text_sect_high
	    = info->interp_text_sect_low
	    + bfd_section_size (tmp_bfd, interp_sect);
	}
      interp_sect = bfd_get_section_by_name (tmp_bfd, ".plt");
      if (interp_sect)
	{
	  info->interp_plt_sect_low =
	    bfd_section_vma (tmp_bfd, interp_sect);
	  info->interp_plt_sect_low
	    += displacement_from_map (ldm, info->interp_plt_sect_low);
	  info->interp_plt_sect_high =
	    info->interp_plt_sect_low + bfd_section_size (tmp_bfd, interp_sect);
	}

      addr = gdb_bfd_lookup_symbol (tmp_bfd, cmp_name, "_dl_debug_addr");
      if (addr == 0)
	{
	  warning (_("Could not find symbol _dl_debug_addr in dynamic linker"));
	  enable_break_failure_warning ();
	  gdb_bfd_unref (tmp_bfd);
	  return 0;
	}

      if (solib_dsbt_debug)
	fprintf_unfiltered (gdb_stdlog,
	                    "enable_break: _dl_debug_addr (prior to relocation) = %s\n",
			    hex_string_custom (addr, 8));

      addr += displacement_from_map (ldm, addr);

      if (solib_dsbt_debug)
	fprintf_unfiltered (gdb_stdlog,
	                    "enable_break: _dl_debug_addr (after relocation) = %s\n",
			    hex_string_custom (addr, 8));

      /* Fetch the address of the r_debug struct.  */
      if (target_read_memory (addr, addr_buf, sizeof addr_buf) != 0)
	{
	  warning (_("Unable to fetch contents of _dl_debug_addr "
		     "(at address %s) from dynamic linker"),
	           hex_string_custom (addr, 8));
	}
      addr = extract_unsigned_integer (addr_buf, sizeof addr_buf, byte_order);

      if (solib_dsbt_debug)
	fprintf_unfiltered (gdb_stdlog,
	                    "enable_break: _dl_debug_addr[0..3] = %s\n",
	                    hex_string_custom (addr, 8));

      /* If it's zero, then the ldso hasn't initialized yet, and so
         there are no shared libs yet loaded.  */
      if (addr == 0)
	{
	  if (solib_dsbt_debug)
	    fprintf_unfiltered (gdb_stdlog,
	                        "enable_break: ldso not yet initialized\n");
	  /* Do not warn, but mark to run again.  */
	  return 0;
	}

      /* Fetch the r_brk field.  It's 8 bytes from the start of
         _dl_debug_addr.  */
      if (target_read_memory (addr + 8, addr_buf, sizeof addr_buf) != 0)
	{
	  warning (_("Unable to fetch _dl_debug_addr->r_brk "
		     "(at address %s) from dynamic linker"),
	           hex_string_custom (addr + 8, 8));
	  enable_break_failure_warning ();
	  gdb_bfd_unref (tmp_bfd);
	  return 0;
	}
      addr = extract_unsigned_integer (addr_buf, sizeof addr_buf, byte_order);

      /* We're done with the temporary bfd.  */
      gdb_bfd_unref (tmp_bfd);

      /* We're also done with the loadmap.  */
      xfree (ldm);

      /* Remove all the solib event breakpoints.  Their addresses
         may have changed since the last time we ran the program.  */
      remove_solib_event_breakpoints ();

      /* Now (finally!) create the solib breakpoint.  */
      create_solib_event_breakpoint (target_gdbarch (), addr);

      info->enable_break2_done = 1;

      return 1;
    }

  /* Tell the user we couldn't set a dynamic linker breakpoint.  */
  enable_break_failure_warning ();

  /* Failure return.  */
  return 0;
}

static int
enable_break (void)
{
  asection *interp_sect;
  struct minimal_symbol *start;

  /* Check for the presence of a .interp section.  If there is no
     such section, the executable is statically linked.  */

  interp_sect = bfd_get_section_by_name (exec_bfd, ".interp");

  if (interp_sect == NULL)
    {
      if (solib_dsbt_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "enable_break: No .interp section found.\n");
      return 0;
    }

  start = lookup_minimal_symbol ("_start", NULL, symfile_objfile);
  if (start == NULL)
    {
      if (solib_dsbt_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "enable_break: symbol _start is not found.\n");
      return 0;
    }

  create_solib_event_breakpoint (target_gdbarch (),
				 SYMBOL_VALUE_ADDRESS (start));

  if (solib_dsbt_debug)
    fprintf_unfiltered (gdb_stdlog,
			"enable_break: solib event breakpoint placed at : %s\n",
			hex_string_custom (SYMBOL_VALUE_ADDRESS (start), 8));
  return 1;
}

/* Once the symbols from a shared object have been loaded in the usual
   way, we are called to do any system specific symbol handling that
   is needed.  */

static void
dsbt_special_symbol_handling (void)
{
}

static void
dsbt_relocate_main_executable (void)
{
  struct int_elf32_dsbt_loadmap *ldm;
  struct cleanup *old_chain;
  struct section_offsets *new_offsets;
  int changed;
  struct obj_section *osect;
  struct dsbt_info *info = get_dsbt_info ();

  dsbt_get_initial_loadmaps ();
  ldm = info->exec_loadmap;

  xfree (info->main_executable_lm_info);
  info->main_executable_lm_info = xcalloc (1, sizeof (struct lm_info));
  info->main_executable_lm_info->map = ldm;

  new_offsets = xcalloc (symfile_objfile->num_sections,
			 sizeof (struct section_offsets));
  old_chain = make_cleanup (xfree, new_offsets);
  changed = 0;

  ALL_OBJFILE_OSECTIONS (symfile_objfile, osect)
    {
      CORE_ADDR orig_addr, addr, offset;
      int osect_idx;
      int seg;

      osect_idx = osect->the_bfd_section->index;

      /* Current address of section.  */
      addr = obj_section_addr (osect);
      /* Offset from where this section started.  */
      offset = ANOFFSET (symfile_objfile->section_offsets, osect_idx);
      /* Original address prior to any past relocations.  */
      orig_addr = addr - offset;

      for (seg = 0; seg < ldm->nsegs; seg++)
	{
	  if (ldm->segs[seg].p_vaddr <= orig_addr
	      && orig_addr < ldm->segs[seg].p_vaddr + ldm->segs[seg].p_memsz)
	    {
	      new_offsets->offsets[osect_idx]
		= ldm->segs[seg].addr - ldm->segs[seg].p_vaddr;

	      if (new_offsets->offsets[osect_idx] != offset)
		changed = 1;
	      break;
	    }
	}
    }

  if (changed)
    objfile_relocate (symfile_objfile, new_offsets);

  do_cleanups (old_chain);

  /* Now that symfile_objfile has been relocated, we can compute the
     GOT value and stash it away.  */
}

/* When gdb starts up the inferior, it nurses it along (through the
   shell) until it is ready to execute it's first instruction.  At this
   point, this function gets called via expansion of the macro
   SOLIB_CREATE_INFERIOR_HOOK.

   For the DSBT shared library, the main executable needs to be relocated.
   The shared library breakpoints also need to be enabled.
 */

static void
dsbt_solib_create_inferior_hook (int from_tty)
{
  /* Relocate main executable.  */
  dsbt_relocate_main_executable ();

  /* Enable shared library breakpoints.  */
  if (!enable_break ())
    {
      warning (_("shared library handler failed to enable breakpoint"));
      return;
    }
}

static void
dsbt_clear_solib (void)
{
  struct dsbt_info *info = get_dsbt_info ();

  info->lm_base_cache = 0;
  info->enable_break2_done = 0;
  info->main_lm_addr = 0;
  if (info->main_executable_lm_info != 0)
    {
      xfree (info->main_executable_lm_info->map);
      xfree (info->main_executable_lm_info);
      info->main_executable_lm_info = 0;
    }
}

static void
dsbt_free_so (struct so_list *so)
{
  xfree (so->lm_info->map);
  xfree (so->lm_info);
}

static void
dsbt_relocate_section_addresses (struct so_list *so,
                                 struct target_section *sec)
{
  int seg;
  struct int_elf32_dsbt_loadmap *map;

  map = so->lm_info->map;

  for (seg = 0; seg < map->nsegs; seg++)
    {
      if (map->segs[seg].p_vaddr <= sec->addr
          && sec->addr < map->segs[seg].p_vaddr + map->segs[seg].p_memsz)
	{
	  CORE_ADDR displ = map->segs[seg].addr - map->segs[seg].p_vaddr;

	  sec->addr += displ;
	  sec->endaddr += displ;
	  break;
	}
    }
}
static void
show_dsbt_debug (struct ui_file *file, int from_tty,
		 struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("solib-dsbt debugging is %s.\n"), value);
}

struct target_so_ops dsbt_so_ops;

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_dsbt_solib;

void
_initialize_dsbt_solib (void)
{
  solib_dsbt_pspace_data
    = register_program_space_data_with_cleanup (NULL, dsbt_pspace_data_cleanup);

  dsbt_so_ops.relocate_section_addresses = dsbt_relocate_section_addresses;
  dsbt_so_ops.free_so = dsbt_free_so;
  dsbt_so_ops.clear_solib = dsbt_clear_solib;
  dsbt_so_ops.solib_create_inferior_hook = dsbt_solib_create_inferior_hook;
  dsbt_so_ops.special_symbol_handling = dsbt_special_symbol_handling;
  dsbt_so_ops.current_sos = dsbt_current_sos;
  dsbt_so_ops.open_symbol_file_object = open_symbol_file_object;
  dsbt_so_ops.in_dynsym_resolve_code = dsbt_in_dynsym_resolve_code;
  dsbt_so_ops.bfd_open = solib_bfd_open;

  /* Debug this file's internals.  */
  add_setshow_zuinteger_cmd ("solib-dsbt", class_maintenance,
			     &solib_dsbt_debug, _("\
Set internal debugging of shared library code for DSBT ELF."), _("\
Show internal debugging of shared library code for DSBT ELF."), _("\
When non-zero, DSBT solib specific internal debugging is enabled."),
			     NULL,
			     show_dsbt_debug,
			     &setdebuglist, &showdebuglist);
}
