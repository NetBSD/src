/* Cell SPU GNU/Linux support -- shared library handling.
   Copyright (C) 2009-2016 Free Software Foundation, Inc.

   Contributed by Ulrich Weigand <uweigand@de.ibm.com>.

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
#include "solib-spu.h"
#include "gdbcore.h"
#include <sys/stat.h>
#include "arch-utils.h"
#include "bfd.h"
#include "symtab.h"
#include "solib.h"
#include "solib-svr4.h"
#include "solist.h"
#include "inferior.h"
#include "objfiles.h"
#include "observer.h"
#include "breakpoint.h"
#include "gdbthread.h"
#include "gdb_bfd.h"

#include "spu-tdep.h"

/* Highest SPE id (file handle) the inferior may have.  */
#define MAX_SPE_FD 1024

/* Stand-alone SPE executable?  */
#define spu_standalone_p() \
  (symfile_objfile && symfile_objfile->obfd \
   && bfd_get_arch (symfile_objfile->obfd) == bfd_arch_spu)


/* Relocate main SPE executable.  */
static void
spu_relocate_main_executable (int spufs_fd)
{
  struct section_offsets *new_offsets;
  int i;

  if (symfile_objfile == NULL)
    return;

  new_offsets = XALLOCAVEC (struct section_offsets,
			    symfile_objfile->num_sections);

  for (i = 0; i < symfile_objfile->num_sections; i++)
    new_offsets->offsets[i] = SPUADDR (spufs_fd, 0);

  objfile_relocate (symfile_objfile, new_offsets);
}

/* When running a stand-alone SPE executable, we may need to skip one more
   exec event on startup, to get past the binfmt_misc loader.  */
static void
spu_skip_standalone_loader (void)
{
  if (target_has_execution && !current_inferior ()->attach_flag)
    {
      struct target_waitstatus ws;

      /* Only some kernels report an extra SIGTRAP with the binfmt_misc
	 loader; others do not.  In addition, if we have attached to an
	 already running inferior instead of starting a new one, we will
	 not see the extra SIGTRAP -- and we cannot readily distinguish
	 the two cases, in particular with the extended-remote target.

	 Thus we issue a single-step here.  If no extra SIGTRAP was pending,
	 this will step past the first instruction of the stand-alone SPE
	 executable loader, but we don't care about that.  */

      inferior_thread ()->control.in_infcall = 1; /* Suppress MI messages.  */

      target_resume (inferior_ptid, 1, GDB_SIGNAL_0);
      target_wait (minus_one_ptid, &ws, 0);
      set_executing (minus_one_ptid, 0);

      inferior_thread ()->control.in_infcall = 0;
    }
}

static const struct objfile_data *ocl_program_data_key;

/* Appends OpenCL programs to the list of `struct so_list' objects.  */
static void
append_ocl_sos (struct so_list **link_ptr)
{
  CORE_ADDR *ocl_program_addr_base;
  struct objfile *objfile;

  ALL_OBJFILES (objfile)
    {
      ocl_program_addr_base
	= (CORE_ADDR *) objfile_data (objfile, ocl_program_data_key);
      if (ocl_program_addr_base != NULL)
        {
	  enum bfd_endian byte_order = bfd_big_endian (objfile->obfd)?
					 BFD_ENDIAN_BIG : BFD_ENDIAN_LITTLE;
	  TRY
	    {
	      CORE_ADDR data =
		read_memory_unsigned_integer (*ocl_program_addr_base,
					      sizeof (CORE_ADDR),
					      byte_order);
	      if (data != 0x0)
		{
		  struct so_list *newobj;

		  /* Allocate so_list structure.  */
		  newobj = XCNEW (struct so_list);

		  /* Encode FD and object ID in path name.  */
		  xsnprintf (newobj->so_name, sizeof newobj->so_name, "@%s <%d>",
			     hex_string (data),
			     SPUADDR_SPU (*ocl_program_addr_base));
		  strcpy (newobj->so_original_name, newobj->so_name);

		  *link_ptr = newobj;
		  link_ptr = &newobj->next;
		}
	    }
	  CATCH (ex, RETURN_MASK_ALL)
	    {
	      /* Ignore memory errors.  */
	      switch (ex.error)
		{
		case MEMORY_ERROR:
		  break;
		default:
		  throw_exception (ex);
		  break;
		}
	    }
	  END_CATCH
	}
    }
}

/* Build a list of `struct so_list' objects describing the shared
   objects currently loaded in the inferior.  */
static struct so_list *
spu_current_sos (void)
{
  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());
  struct so_list *head;
  struct so_list **link_ptr;

  gdb_byte buf[MAX_SPE_FD * 4];
  int i, size;

  /* First, retrieve the SVR4 shared library list.  */
  head = svr4_so_ops.current_sos ();

  /* Append our libraries to the end of the list.  */
  for (link_ptr = &head; *link_ptr; link_ptr = &(*link_ptr)->next)
    ;

  /* Determine list of SPU ids.  */
  size = target_read (&current_target, TARGET_OBJECT_SPU, NULL,
		      buf, 0, sizeof buf);

  /* Do not add stand-alone SPE executable context as shared library,
     but relocate main SPE executable objfile.  */
  if (spu_standalone_p ())
    {
      if (size == 4)
	{
	  int fd = extract_unsigned_integer (buf, 4, byte_order);

	  spu_relocate_main_executable (fd);

	  /* Re-enable breakpoints after main SPU context was established;
	     see also comments in spu_solib_create_inferior_hook.  */
	  enable_breakpoints_after_startup ();
	}

      return head;
    }

  /* Create an so_list entry for each SPU id.  */
  for (i = 0; i < size; i += 4)
    {
      int fd = extract_unsigned_integer (buf + i, 4, byte_order);
      struct so_list *newobj;

      unsigned long long addr;
      char annex[32], id[100];
      int len;

      /* Read object ID.  There's a race window where the inferior may have
	 already created the SPE context, but not installed the object-id
	 yet.  Skip such entries; we'll be back for them later.  */
      xsnprintf (annex, sizeof annex, "%d/object-id", fd);
      len = target_read (&current_target, TARGET_OBJECT_SPU, annex,
			 (gdb_byte *) id, 0, sizeof id);
      if (len <= 0 || len >= sizeof id)
	continue;
      id[len] = 0;
      if (sscanf (id, "0x%llx", &addr) != 1 || !addr)
	continue;

      /* Allocate so_list structure.  */
      newobj = XCNEW (struct so_list);

      /* Encode FD and object ID in path name.  Choose the name so as not
	 to conflict with any (normal) SVR4 library path name.  */
      xsnprintf (newobj->so_name, sizeof newobj->so_name, "@%s <%d>",
		 hex_string (addr), fd);
      strcpy (newobj->so_original_name, newobj->so_name);

      *link_ptr = newobj;
      link_ptr = &newobj->next;
    }

  /* Append OpenCL sos.  */
  append_ocl_sos (link_ptr);

  return head;
}

/* Free so_list information.  */
static void
spu_free_so (struct so_list *so)
{
  if (so->so_original_name[0] != '@')
    svr4_so_ops.free_so (so);
}

/* Relocate section addresses.  */
static void
spu_relocate_section_addresses (struct so_list *so,
				struct target_section *sec)
{
  if (so->so_original_name[0] != '@')
    svr4_so_ops.relocate_section_addresses (so, sec);
  else
    {
      unsigned long long addr;
      int fd;

      /* Set addr_low/high to just LS offset for display.  */
      if (so->addr_low == 0 && so->addr_high == 0
          && strcmp (sec->the_bfd_section->name, ".text") == 0)
        {
          so->addr_low = sec->addr;
          so->addr_high = sec->endaddr;
        }

      /* Decode object ID.  */
      if (sscanf (so->so_original_name, "@0x%llx <%d>", &addr, &fd) != 2)
	internal_error (__FILE__, __LINE__, "bad object ID");

      sec->addr = SPUADDR (fd, sec->addr);
      sec->endaddr = SPUADDR (fd, sec->endaddr);
    }
}


/* Inferior memory should contain an SPE executable image at location ADDR.
   Allocate a BFD representing that executable.  Return NULL on error.  */

static void *
spu_bfd_iovec_open (bfd *nbfd, void *open_closure)
{
  return open_closure;
}

static int
spu_bfd_iovec_close (bfd *nbfd, void *stream)
{
  xfree (stream);

  /* Zero means success.  */
  return 0;
}

static file_ptr
spu_bfd_iovec_pread (bfd *abfd, void *stream, void *buf,
                     file_ptr nbytes, file_ptr offset)
{
  CORE_ADDR addr = *(CORE_ADDR *)stream;
  int ret;

  ret = target_read_memory (addr + offset, (gdb_byte *) buf, nbytes);
  if (ret != 0)
    {
      bfd_set_error (bfd_error_invalid_operation);
      return -1;
    }

  return nbytes;
}

static int
spu_bfd_iovec_stat (bfd *abfd, void *stream, struct stat *sb)
{
  /* We don't have an easy way of finding the size of embedded spu
     images.  We could parse the in-memory ELF header and section
     table to find the extent of the last section but that seems
     pointless when the size is needed only for checks of other
     parsed values in dbxread.c.  */
  memset (sb, 0, sizeof (struct stat));
  sb->st_size = INT_MAX;
  return 0;
}

static bfd *
spu_bfd_fopen (char *name, CORE_ADDR addr)
{
  bfd *nbfd;
  CORE_ADDR *open_closure = XNEW (CORE_ADDR);

  *open_closure = addr;

  nbfd = gdb_bfd_openr_iovec (name, "elf32-spu",
			      spu_bfd_iovec_open, open_closure,
			      spu_bfd_iovec_pread, spu_bfd_iovec_close,
			      spu_bfd_iovec_stat);
  if (!nbfd)
    return NULL;

  if (!bfd_check_format (nbfd, bfd_object))
    {
      gdb_bfd_unref (nbfd);
      return NULL;
    }

  return nbfd;
}

/* Open shared library BFD.  */
static bfd *
spu_bfd_open (char *pathname)
{
  char *original_name = strrchr (pathname, '@');
  bfd *abfd;
  asection *spu_name;
  unsigned long long addr;
  int fd;

  /* Handle regular SVR4 libraries.  */
  if (!original_name)
    return svr4_so_ops.bfd_open (pathname);

  /* Decode object ID.  */
  if (sscanf (original_name, "@0x%llx <%d>", &addr, &fd) != 2)
    internal_error (__FILE__, __LINE__, "bad object ID");

  /* Open BFD representing SPE executable.  */
  abfd = spu_bfd_fopen (original_name, (CORE_ADDR) addr);
  if (!abfd)
    error (_("Cannot read SPE executable at %s"), original_name);

  /* Retrieve SPU name note.  */
  spu_name = bfd_get_section_by_name (abfd, ".note.spu_name");
  if (spu_name)
    {
      int sect_size = bfd_section_size (abfd, spu_name);

      if (sect_size > 20)
	{
	  char *buf
	    = (char *) alloca (sect_size - 20 + strlen (original_name) + 1);

	  bfd_get_section_contents (abfd, spu_name, buf, 20, sect_size - 20);
	  buf[sect_size - 20] = '\0';

	  strcat (buf, original_name);

	  xfree ((char *)abfd->filename);
	  abfd->filename = xstrdup (buf);
	}
    }

  return abfd;
}

/* Lookup global symbol in a SPE executable.  */
static struct block_symbol
spu_lookup_lib_symbol (struct objfile *objfile,
		       const char *name,
		       const domain_enum domain)
{
  if (bfd_get_arch (objfile->obfd) == bfd_arch_spu)
    return lookup_global_symbol_from_objfile (objfile, name, domain);

  if (svr4_so_ops.lookup_lib_global_symbol != NULL)
    return svr4_so_ops.lookup_lib_global_symbol (objfile, name, domain);
  return (struct block_symbol) {NULL, NULL};
}

/* Enable shared library breakpoint.  */
static int
spu_enable_break (struct objfile *objfile)
{
  struct bound_minimal_symbol spe_event_sym;

  /* The libspe library will call __spe_context_update_event whenever any
     SPE context is allocated or destroyed.  */
  spe_event_sym = lookup_minimal_symbol ("__spe_context_update_event",
					 NULL, objfile);

  /* Place a solib_event breakpoint on the symbol.  */
  if (spe_event_sym.minsym)
    {
      CORE_ADDR addr = BMSYMBOL_VALUE_ADDRESS (spe_event_sym);

      addr = gdbarch_convert_from_func_ptr_addr (target_gdbarch (), addr,
                                                 &current_target);
      create_solib_event_breakpoint (target_gdbarch (), addr);
      return 1;
    }

  return 0;
}

/* Enable shared library breakpoint for the
   OpenCL runtime running on the SPU.  */
static void
ocl_enable_break (struct objfile *objfile)
{
  struct bound_minimal_symbol event_sym;
  struct bound_minimal_symbol addr_sym;

  /* The OpenCL runtime on the SPU will call __opencl_program_update_event
     whenever an OpenCL program is loaded.  */
  event_sym = lookup_minimal_symbol ("__opencl_program_update_event", NULL,
				     objfile);
  /* The PPU address of the OpenCL program can be found
     at opencl_elf_image_address.  */
  addr_sym = lookup_minimal_symbol ("opencl_elf_image_address", NULL, objfile);

  if (event_sym.minsym && addr_sym.minsym)
    {
      /* Place a solib_event breakpoint on the symbol.  */
      CORE_ADDR event_addr = BMSYMBOL_VALUE_ADDRESS (event_sym);
      create_solib_event_breakpoint (get_objfile_arch (objfile), event_addr);

      /* Store the address of the symbol that will point to OpenCL program
         using the per-objfile private data mechanism.  */
      if (objfile_data (objfile, ocl_program_data_key) == NULL)
        {
          CORE_ADDR *ocl_program_addr_base = OBSTACK_CALLOC (
		  &objfile->objfile_obstack,
		  objfile->sections_end - objfile->sections,
		  CORE_ADDR);
	  *ocl_program_addr_base = BMSYMBOL_VALUE_ADDRESS (addr_sym);
	  set_objfile_data (objfile, ocl_program_data_key,
			    ocl_program_addr_base);
        }
    }
}

/* Create inferior hook.  */
static void
spu_solib_create_inferior_hook (int from_tty)
{
  /* Handle SPE stand-alone executables.  */
  if (spu_standalone_p ())
    {
      /* After an SPE stand-alone executable was loaded, we'll receive
	 an additional trap due to the binfmt_misc handler.  Make sure
	 to skip that trap.  */
      spu_skip_standalone_loader ();

      /* If the user established breakpoints before starting the inferior, GDB
	 would attempt to insert those now.  This would fail because the SPU
	 context has not yet been created and the SPU executable has not yet
	 been loaded.  To prevent such failures, we disable all user-created
	 breakpoints now; they will be re-enabled in spu_current_sos once the
	 main SPU context has been detected.  */
      disable_breakpoints_before_startup ();

      /* A special case arises when re-starting an executable, because at
	 this point it still resides at the relocated address range that was
	 determined during its last execution.  We need to undo the relocation
	 so that that multi-architecture target recognizes the stand-alone
	 initialization special case.  */
      spu_relocate_main_executable (-1);
    }

  /* Call SVR4 hook -- this will re-insert the SVR4 solib breakpoints.  */
  svr4_so_ops.solib_create_inferior_hook (from_tty);

  /* If the inferior is statically linked against libspe, we need to install
     our own solib breakpoint right now.  Otherwise, it will be installed by
     the solib_loaded observer below as soon as libspe is loaded.  */
  spu_enable_break (NULL);
}

/* Install SPE "shared library" handling.  This is called by -tdep code
   that wants to support SPU as a secondary architecture.  */
void
set_spu_solib_ops (struct gdbarch *gdbarch)
{
  static struct target_so_ops spu_so_ops;

  /* Initialize this lazily, to avoid an initialization order
     dependency on solib-svr4.c's _initialize routine.  */
  if (spu_so_ops.current_sos == NULL)
    {
      spu_so_ops = svr4_so_ops;
      spu_so_ops.solib_create_inferior_hook = spu_solib_create_inferior_hook;
      spu_so_ops.relocate_section_addresses = spu_relocate_section_addresses;
      spu_so_ops.free_so = spu_free_so;
      spu_so_ops.current_sos = spu_current_sos;
      spu_so_ops.bfd_open = spu_bfd_open;
      spu_so_ops.lookup_lib_global_symbol = spu_lookup_lib_symbol;
    }

  set_solib_ops (gdbarch, &spu_so_ops);
}

/* Observer for the solib_loaded event.  Used to install our breakpoint
   if libspe is a shared library.  */
static void
spu_solib_loaded (struct so_list *so)
{
  if (strstr (so->so_original_name, "/libspe") != NULL)
    {
      solib_read_symbols (so, 0);
      spu_enable_break (so->objfile);
    }
  /* In case the OpenCL runtime is loaded we install a breakpoint
     to get notified whenever an OpenCL program gets loaded.  */
  if (strstr (so->so_name, "CLRuntimeAccelCellSPU@") != NULL)
    {
      solib_read_symbols (so, 0);
      ocl_enable_break (so->objfile);
    }
}

/* -Wmissing-prototypes */
extern initialize_file_ftype _initialize_spu_solib;

void
_initialize_spu_solib (void)
{
  observer_attach_solib_loaded (spu_solib_loaded);
  ocl_program_data_key = register_objfile_data ();
}

