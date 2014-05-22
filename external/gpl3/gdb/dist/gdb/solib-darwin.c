/* Handle Darwin shared libraries for GDB, the GNU Debugger.

   Copyright (C) 2009-2013 Free Software Foundation, Inc.

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

#include "symtab.h"
#include "bfd.h"
#include "symfile.h"
#include "objfiles.h"
#include "gdbcore.h"
#include "target.h"
#include "inferior.h"
#include "regcache.h"
#include "gdbthread.h"
#include "gdb_bfd.h"

#include "gdb_assert.h"

#include "solist.h"
#include "solib.h"
#include "solib-svr4.h"

#include "bfd-target.h"
#include "elf-bfd.h"
#include "exec.h"
#include "auxv.h"
#include "exceptions.h"
#include "mach-o.h"
#include "mach-o/external.h"

struct gdb_dyld_image_info
{
  /* Base address (which corresponds to the Mach-O header).  */
  CORE_ADDR mach_header;
  /* Image file path.  */
  CORE_ADDR file_path;
  /* st.m_time of image file.  */
  unsigned long mtime;
};

/* Content of inferior dyld_all_image_infos structure.
   See /usr/include/mach-o/dyld_images.h for the documentation.  */
struct gdb_dyld_all_image_infos
{
  /* Version (1).  */
  unsigned int version;
  /* Number of images.  */
  unsigned int count;
  /* Image description.  */
  CORE_ADDR info;
  /* Notifier (function called when a library is added or removed).  */
  CORE_ADDR notifier;
};

/* Current all_image_infos version.  */
#define DYLD_VERSION_MIN 1
#define DYLD_VERSION_MAX 12

/* Per PSPACE specific data.  */
struct darwin_info
{
  /* Address of structure dyld_all_image_infos in inferior.  */
  CORE_ADDR all_image_addr;

  /* Gdb copy of dyld_all_info_infos.  */
  struct gdb_dyld_all_image_infos all_image;
};

/* Per-program-space data key.  */
static const struct program_space_data *solib_darwin_pspace_data;

static void
darwin_pspace_data_cleanup (struct program_space *pspace, void *arg)
{
  struct darwin_info *info;

  info = program_space_data (pspace, solib_darwin_pspace_data);
  xfree (info);
}

/* Get the current darwin data.  If none is found yet, add it now.  This
   function always returns a valid object.  */

static struct darwin_info *
get_darwin_info (void)
{
  struct darwin_info *info;

  info = program_space_data (current_program_space, solib_darwin_pspace_data);
  if (info != NULL)
    return info;

  info = XZALLOC (struct darwin_info);
  set_program_space_data (current_program_space,
			  solib_darwin_pspace_data, info);
  return info;
}

/* Return non-zero if the version in dyld_all_image is known.  */

static int
darwin_dyld_version_ok (const struct darwin_info *info)
{
  return info->all_image.version >= DYLD_VERSION_MIN
    && info->all_image.version <= DYLD_VERSION_MAX;
}

/* Read dyld_all_image from inferior.  */

static void
darwin_load_image_infos (struct darwin_info *info)
{
  gdb_byte buf[24];
  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());
  struct type *ptr_type = builtin_type (target_gdbarch ())->builtin_data_ptr;
  int len;

  /* If the structure address is not known, don't continue.  */
  if (info->all_image_addr == 0)
    return;

  /* The structure has 4 fields: version (4 bytes), count (4 bytes),
     info (pointer) and notifier (pointer).  */
  len = 4 + 4 + 2 * ptr_type->length;
  gdb_assert (len <= sizeof (buf));
  memset (&info->all_image, 0, sizeof (info->all_image));

  /* Read structure raw bytes from target.  */
  if (target_read_memory (info->all_image_addr, buf, len))
    return;

  /* Extract the fields.  */
  info->all_image.version = extract_unsigned_integer (buf, 4, byte_order);
  if (!darwin_dyld_version_ok (info))
    return;

  info->all_image.count = extract_unsigned_integer (buf + 4, 4, byte_order);
  info->all_image.info = extract_typed_address (buf + 8, ptr_type);
  info->all_image.notifier = extract_typed_address
    (buf + 8 + ptr_type->length, ptr_type);
}

/* Link map info to include in an allocated so_list entry.  */

struct lm_info
{
  /* The target location of lm.  */
  CORE_ADDR lm_addr;
};

struct darwin_so_list
{
  /* Common field.  */
  struct so_list sl;
  /* Darwin specific data.  */
  struct lm_info li;
};

/* Lookup the value for a specific symbol.  */

static CORE_ADDR
lookup_symbol_from_bfd (bfd *abfd, char *symname)
{
  long storage_needed;
  asymbol **symbol_table;
  unsigned int number_of_symbols;
  unsigned int i;
  CORE_ADDR symaddr = 0;

  storage_needed = bfd_get_symtab_upper_bound (abfd);

  if (storage_needed <= 0)
    return 0;

  symbol_table = (asymbol **) xmalloc (storage_needed);
  number_of_symbols = bfd_canonicalize_symtab (abfd, symbol_table);

  for (i = 0; i < number_of_symbols; i++)
    {
      asymbol *sym = symbol_table[i];

      if (strcmp (sym->name, symname) == 0
	  && (sym->section->flags & (SEC_CODE | SEC_DATA)) != 0)
	{
	  /* BFD symbols are section relative.  */
	  symaddr = sym->value + sym->section->vma;
	  break;
	}
    }
  xfree (symbol_table);

  return symaddr;
}

/* Return program interpreter string.  */

static gdb_byte *
find_program_interpreter (void)
{
  gdb_byte *buf = NULL;

  /* If we have an exec_bfd, get the interpreter from the load commands.  */
  if (exec_bfd)
    {
      bfd_mach_o_load_command *cmd;

      if (bfd_mach_o_lookup_command (exec_bfd,
                                     BFD_MACH_O_LC_LOAD_DYLINKER, &cmd) == 1)
        return cmd->command.dylinker.name_str;
    }

  /* If we didn't find it, read from memory.
     FIXME: todo.  */
  return buf;
}

/*  Not used.  I don't see how the main symbol file can be found: the
    interpreter name is needed and it is known from the executable file.
    Note that darwin-nat.c implements pid_to_exec_file.  */

static int
open_symbol_file_object (void *from_ttyp)
{
  return 0;
}

/* Build a list of currently loaded shared objects.  See solib-svr4.c.  */

static struct so_list *
darwin_current_sos (void)
{
  struct type *ptr_type = builtin_type (target_gdbarch ())->builtin_data_ptr;
  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());
  int ptr_len = TYPE_LENGTH (ptr_type);
  unsigned int image_info_size;
  struct so_list *head = NULL;
  struct so_list *tail = NULL;
  int i;
  struct darwin_info *info = get_darwin_info ();

  /* Be sure image infos are loaded.  */
  darwin_load_image_infos (info);

  if (!darwin_dyld_version_ok (info))
    return NULL;

  image_info_size = ptr_len * 3;

  /* Read infos for each solib.
     The first entry was rumored to be the executable itself, but this is not
     true when a large number of shared libraries are used (table expanded ?).
     We now check all entries, but discard executable images.  */
  for (i = 0; i < info->all_image.count; i++)
    {
      CORE_ADDR iinfo = info->all_image.info + i * image_info_size;
      gdb_byte buf[image_info_size];
      CORE_ADDR load_addr;
      CORE_ADDR path_addr;
      struct mach_o_header_external hdr;
      unsigned long hdr_val;
      char *file_path;
      int errcode;
      struct darwin_so_list *dnew;
      struct so_list *new;
      struct cleanup *old_chain;

      /* Read image info from inferior.  */
      if (target_read_memory (iinfo, buf, image_info_size))
	break;

      load_addr = extract_typed_address (buf, ptr_type);
      path_addr = extract_typed_address (buf + ptr_len, ptr_type);

      /* Read Mach-O header from memory.  */
      if (target_read_memory (load_addr, (char *) &hdr, sizeof (hdr) - 4))
	break;
      /* Discard wrong magic numbers.  Shouldn't happen.  */
      hdr_val = extract_unsigned_integer
        (hdr.magic, sizeof (hdr.magic), byte_order);
      if (hdr_val != BFD_MACH_O_MH_MAGIC && hdr_val != BFD_MACH_O_MH_MAGIC_64)
        continue;
      /* Discard executable.  Should happen only once.  */
      hdr_val = extract_unsigned_integer
        (hdr.filetype, sizeof (hdr.filetype), byte_order);
      if (hdr_val == BFD_MACH_O_MH_EXECUTE)
        continue;

      target_read_string (path_addr, &file_path,
			  SO_NAME_MAX_PATH_SIZE - 1, &errcode);
      if (errcode)
	break;

      /* Create and fill the new so_list element.  */
      dnew = XZALLOC (struct darwin_so_list);
      new = &dnew->sl;
      old_chain = make_cleanup (xfree, dnew);

      new->lm_info = &dnew->li;

      strncpy (new->so_name, file_path, SO_NAME_MAX_PATH_SIZE - 1);
      new->so_name[SO_NAME_MAX_PATH_SIZE - 1] = '\0';
      strcpy (new->so_original_name, new->so_name);
      xfree (file_path);
      new->lm_info->lm_addr = load_addr;

      if (head == NULL)
	head = new;
      else
	tail->next = new;
      tail = new;

      discard_cleanups (old_chain);
    }

  return head;
}

/* Get the load address of the executable.  We assume that the dyld info are
   correct.  */

static CORE_ADDR
darwin_read_exec_load_addr (struct darwin_info *info)
{
  struct type *ptr_type = builtin_type (target_gdbarch ())->builtin_data_ptr;
  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());
  int ptr_len = TYPE_LENGTH (ptr_type);
  unsigned int image_info_size = ptr_len * 3;
  int i;

  /* Read infos for each solib.  One of them should be the executable.  */
  for (i = 0; i < info->all_image.count; i++)
    {
      CORE_ADDR iinfo = info->all_image.info + i * image_info_size;
      gdb_byte buf[image_info_size];
      CORE_ADDR load_addr;
      struct mach_o_header_external hdr;
      unsigned long hdr_val;

      /* Read image info from inferior.  */
      if (target_read_memory (iinfo, buf, image_info_size))
	break;

      load_addr = extract_typed_address (buf, ptr_type);

      /* Read Mach-O header from memory.  */
      if (target_read_memory (load_addr, (char *) &hdr, sizeof (hdr) - 4))
	break;
      /* Discard wrong magic numbers.  Shouldn't happen.  */
      hdr_val = extract_unsigned_integer
        (hdr.magic, sizeof (hdr.magic), byte_order);
      if (hdr_val != BFD_MACH_O_MH_MAGIC && hdr_val != BFD_MACH_O_MH_MAGIC_64)
        continue;
      /* Check executable.  */
      hdr_val = extract_unsigned_integer
        (hdr.filetype, sizeof (hdr.filetype), byte_order);
      if (hdr_val == BFD_MACH_O_MH_EXECUTE)
	return load_addr;
    }

  return 0;
}

/* Return 1 if PC lies in the dynamic symbol resolution code of the
   run time loader.  */

static int
darwin_in_dynsym_resolve_code (CORE_ADDR pc)
{
  return 0;
}


/* No special symbol handling.  */

static void
darwin_special_symbol_handling (void)
{
}

/* A wrapper for bfd_mach_o_fat_extract that handles reference
   counting properly.  This will either return NULL, or return a new
   reference to a BFD.  */

static bfd *
gdb_bfd_mach_o_fat_extract (bfd *abfd, bfd_format format,
			    const bfd_arch_info_type *arch)
{
  bfd *result = bfd_mach_o_fat_extract (abfd, format, arch);

  if (result == NULL)
    return NULL;

  if (result == abfd)
    gdb_bfd_ref (result);
  else
    gdb_bfd_mark_parent (result, abfd);

  return result;
}

/* Extract dyld_all_image_addr when the process was just created, assuming the
   current PC is at the entry of the dynamic linker.  */

static void
darwin_solib_get_all_image_info_addr_at_init (struct darwin_info *info)
{
  gdb_byte *interp_name;
  CORE_ADDR load_addr = 0;
  bfd *dyld_bfd = NULL;
  struct cleanup *cleanup;

  /* This method doesn't work with an attached process.  */
  if (current_inferior ()->attach_flag)
    return;

  /* Find the program interpreter.  */
  interp_name = find_program_interpreter ();
  if (!interp_name)
    return;

  cleanup = make_cleanup (null_cleanup, NULL);

  /* Create a bfd for the interpreter.  */
  dyld_bfd = gdb_bfd_open (interp_name, gnutarget, -1);
  if (dyld_bfd)
    {
      bfd *sub;

      make_cleanup_bfd_unref (dyld_bfd);
      sub = gdb_bfd_mach_o_fat_extract (dyld_bfd, bfd_object,
					gdbarch_bfd_arch_info (target_gdbarch ()));
      if (sub)
	{
	  dyld_bfd = sub;
	  make_cleanup_bfd_unref (sub);
	}
      else
	dyld_bfd = NULL;
    }
  if (!dyld_bfd)
    {
      do_cleanups (cleanup);
      return;
    }

  /* We find the dynamic linker's base address by examining
     the current pc (which should point at the entry point for the
     dynamic linker) and subtracting the offset of the entry point.  */
  load_addr = (regcache_read_pc (get_current_regcache ())
               - bfd_get_start_address (dyld_bfd));

  /* Now try to set a breakpoint in the dynamic linker.  */
  info->all_image_addr =
    lookup_symbol_from_bfd (dyld_bfd, "_dyld_all_image_infos");

  do_cleanups (cleanup);

  if (info->all_image_addr == 0)
    return;

  info->all_image_addr += load_addr;
}

/* Extract dyld_all_image_addr reading it from 
   TARGET_OBJECT_DARWIN_DYLD_INFO.  */

static void
darwin_solib_read_all_image_info_addr (struct darwin_info *info)
{
  gdb_byte buf[8 + 8 + 4];
  LONGEST len;
  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());

  len = target_read (&current_target, TARGET_OBJECT_DARWIN_DYLD_INFO, NULL,
                     buf, 0, sizeof (buf));
  if (len != sizeof (buf))
    return;

  info->all_image_addr = extract_unsigned_integer (buf, 8, byte_order);
}

/* Shared library startup support.  See documentation in solib-svr4.c.  */

static void
darwin_solib_create_inferior_hook (int from_tty)
{
  struct darwin_info *info = get_darwin_info ();
  CORE_ADDR load_addr;

  info->all_image_addr = 0;

  darwin_solib_read_all_image_info_addr (info);

  if (info->all_image_addr == 0)
    darwin_solib_get_all_image_info_addr_at_init (info);

  if (info->all_image_addr == 0)
    return;

  darwin_load_image_infos (info);

  if (!darwin_dyld_version_ok (info))
    return;

  create_solib_event_breakpoint (target_gdbarch (), info->all_image.notifier);

  /* Possible relocate the main executable (PIE).  */
  load_addr = darwin_read_exec_load_addr (info);
  if (load_addr != 0 && symfile_objfile != NULL)
    {
      CORE_ADDR vmaddr = 0;
      struct mach_o_data_struct *md = bfd_mach_o_get_data (exec_bfd);
      unsigned int i, num;

      /* Find the base address of the executable.  */
      for (i = 0; i < md->header.ncmds; i++)
	{
	  struct bfd_mach_o_load_command *cmd = &md->commands[i];

	  if (cmd->type != BFD_MACH_O_LC_SEGMENT
	      && cmd->type != BFD_MACH_O_LC_SEGMENT_64)
	    continue;
	  if (cmd->command.segment.fileoff == 0
	      && cmd->command.segment.vmaddr != 0
	      && cmd->command.segment.filesize != 0)
	    {
	      vmaddr = cmd->command.segment.vmaddr;
	      break;
	    }
	}

      /* Relocate.  */
      if (vmaddr != load_addr)
	objfile_rebase (symfile_objfile, load_addr - vmaddr);
    }
}

static void
darwin_clear_solib (void)
{
  struct darwin_info *info = get_darwin_info ();

  info->all_image_addr = 0;
  info->all_image.version = 0;
}

static void
darwin_free_so (struct so_list *so)
{
}

/* The section table is built from bfd sections using bfd VMAs.
   Relocate these VMAs according to solib info.  */

static void
darwin_relocate_section_addresses (struct so_list *so,
				   struct target_section *sec)
{
  sec->addr += so->lm_info->lm_addr;
  sec->endaddr += so->lm_info->lm_addr;

  /* Best effort to set addr_high/addr_low.  This is used only by
     'info sharedlibary'.  */
  if (so->addr_high == 0)
    {
      so->addr_low = sec->addr;
      so->addr_high = sec->endaddr;
    }
  if (sec->endaddr > so->addr_high)
    so->addr_high = sec->endaddr;
  if (sec->addr < so->addr_low)
    so->addr_low = sec->addr;
}

static struct symbol *
darwin_lookup_lib_symbol (const struct objfile *objfile,
			  const char *name,
			  const domain_enum domain)
{
  return NULL;
}

static bfd *
darwin_bfd_open (char *pathname)
{
  char *found_pathname;
  int found_file;
  bfd *abfd;
  bfd *res;

  /* Search for shared library file.  */
  found_pathname = solib_find (pathname, &found_file);
  if (found_pathname == NULL)
    perror_with_name (pathname);

  /* Open bfd for shared library.  */
  abfd = solib_bfd_fopen (found_pathname, found_file);

  res = gdb_bfd_mach_o_fat_extract (abfd, bfd_object,
				    gdbarch_bfd_arch_info (target_gdbarch ()));
  if (!res)
    {
      make_cleanup_bfd_unref (abfd);
      error (_("`%s': not a shared-library: %s"),
	     bfd_get_filename (abfd), bfd_errmsg (bfd_get_error ()));
    }

  /* The current filename for fat-binary BFDs is a name generated
     by BFD, usually a string containing the name of the architecture.
     Reset its value to the actual filename.  */
    {
      char *data = bfd_alloc (res, strlen (pathname) + 1);

      strcpy (data, pathname);
      res->filename = data;
    }

  gdb_bfd_unref (abfd);
  return res;
}

struct target_so_ops darwin_so_ops;

/* -Wmissing-prototypes */
extern initialize_file_ftype _initialize_darwin_solib;

void
_initialize_darwin_solib (void)
{
  solib_darwin_pspace_data
    = register_program_space_data_with_cleanup (NULL,
						darwin_pspace_data_cleanup);

  darwin_so_ops.relocate_section_addresses = darwin_relocate_section_addresses;
  darwin_so_ops.free_so = darwin_free_so;
  darwin_so_ops.clear_solib = darwin_clear_solib;
  darwin_so_ops.solib_create_inferior_hook = darwin_solib_create_inferior_hook;
  darwin_so_ops.special_symbol_handling = darwin_special_symbol_handling;
  darwin_so_ops.current_sos = darwin_current_sos;
  darwin_so_ops.open_symbol_file_object = open_symbol_file_object;
  darwin_so_ops.in_dynsym_resolve_code = darwin_in_dynsym_resolve_code;
  darwin_so_ops.lookup_lib_global_symbol = darwin_lookup_lib_symbol;
  darwin_so_ops.bfd_open = darwin_bfd_open;
}
