/* Handle SunOS shared libraries for GDB, the GNU Debugger.

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

#include <sys/types.h>
#include <signal.h>
#include "gdb_string.h"
#include <sys/param.h>
#include <fcntl.h>

/* SunOS shared libs need the nlist structure.  */
#include <a.out.h>
#include <link.h>

#include "symtab.h"
#include "bfd.h"
#include "symfile.h"
#include "objfiles.h"
#include "gdbcore.h"
#include "inferior.h"
#include "gdbthread.h"
#include "solist.h"
#include "bcache.h"
#include "regcache.h"

/* The shared library implementation found on BSD a.out systems is
   very similar to the SunOS implementation.  However, the data
   structures defined in <link.h> are named very differently.  Make up
   for those differences here.  */

#ifdef HAVE_STRUCT_SO_MAP_WITH_SOM_MEMBERS

/* FIXME: Temporary until the equivalent defines have been removed
   from all nm-*bsd*.h files.  */
#ifndef link_dynamic

/* Map `struct link_map' and its members.  */
#define link_map	so_map
#define lm_addr		som_addr
#define lm_name		som_path
#define lm_next		som_next

/* Map `struct link_dynamic_2' and its members.  */
#define link_dynamic_2	section_dispatch_table
#define ld_loaded	sdt_loaded

/* Map `struct rtc_symb' and its members.  */
#define rtc_symb	rt_symbol
#define rtc_sp		rt_sp
#define rtc_next	rt_next

/* Map `struct ld_debug' and its members.  */
#define ld_debug	so_debug
#define ldd_in_debugger	dd_in_debugger
#define ldd_bp_addr	dd_bpt_addr
#define ldd_bp_inst	dd_bpt_shadow
#define ldd_cp		dd_cc

/* Map `struct link_dynamic' and its members.  */
#define link_dynamic	_dynamic
#define ld_version	d_version
#define ldd		d_debug
#define ld_un		d_un
#define ld_2		d_sdt

#endif

#endif

/* Link map info to include in an allocated so_list entry.  */

struct lm_info
  {
    /* Pointer to copy of link map from inferior.  The type is char *
       rather than void *, so that we may use byte offsets to find the
       various fields without the need for a cast.  */
    char *lm;
  };


/* Symbols which are used to locate the base of the link map structures.  */

static char *debug_base_symbols[] =
{
  "_DYNAMIC",
  "_DYNAMIC__MGC",
  NULL
};

static char *main_name_list[] =
{
  "main_$main",
  NULL
};

/* Macro to extract an address from a solib structure.  When GDB is
   configured for some 32-bit targets (e.g. Solaris 2.7 sparc), BFD is
   configured to handle 64-bit targets, so CORE_ADDR is 64 bits.  We
   have to extract only the significant bits of addresses to get the
   right address when accessing the core file BFD.

   Assume that the address is unsigned.  */

#define SOLIB_EXTRACT_ADDRESS(MEMBER) \
	extract_unsigned_integer (&(MEMBER), sizeof (MEMBER), \
				  gdbarch_byte_order (target_gdbarch ()))

/* local data declarations */

static struct link_dynamic dynamic_copy;
static struct link_dynamic_2 ld_2_copy;
static struct ld_debug debug_copy;
static CORE_ADDR debug_addr;
static CORE_ADDR flag_addr;

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((unsigned long) &((TYPE *)0)->MEMBER)
#endif
#define fieldsize(TYPE, MEMBER) (sizeof (((TYPE *)0)->MEMBER))

/* link map access functions */

static CORE_ADDR
lm_addr (struct so_list *so)
{
  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());
  int lm_addr_offset = offsetof (struct link_map, lm_addr);
  int lm_addr_size = fieldsize (struct link_map, lm_addr);

  return (CORE_ADDR) extract_signed_integer (so->lm_info->lm + lm_addr_offset, 
					     lm_addr_size, byte_order);
}

static CORE_ADDR
lm_next (struct so_list *so)
{
  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());
  int lm_next_offset = offsetof (struct link_map, lm_next);
  int lm_next_size = fieldsize (struct link_map, lm_next);

  /* Assume that the address is unsigned.  */
  return extract_unsigned_integer (so->lm_info->lm + lm_next_offset,
				   lm_next_size, byte_order);
}

static CORE_ADDR
lm_name (struct so_list *so)
{
  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());
  int lm_name_offset = offsetof (struct link_map, lm_name);
  int lm_name_size = fieldsize (struct link_map, lm_name);

  /* Assume that the address is unsigned.  */
  return extract_unsigned_integer (so->lm_info->lm + lm_name_offset,
				   lm_name_size, byte_order);
}

static CORE_ADDR debug_base;	/* Base of dynamic linker structures.  */

/* Local function prototypes */

static int match_main (char *);

/* Allocate the runtime common object file.  */

static void
allocate_rt_common_objfile (void)
{
  struct objfile *objfile;
  struct objfile *last_one;

  objfile = (struct objfile *) xmalloc (sizeof (struct objfile));
  memset (objfile, 0, sizeof (struct objfile));
  objfile->psymbol_cache = psymbol_bcache_init ();
  objfile->macro_cache = bcache_xmalloc (NULL, NULL);
  objfile->filename_cache = bcache_xmalloc (NULL, NULL);
  obstack_init (&objfile->objfile_obstack);
  objfile->name = xstrdup ("rt_common");

  /* Add this file onto the tail of the linked list of other such files.  */

  objfile->next = NULL;
  if (object_files == NULL)
    object_files = objfile;
  else
    {
      for (last_one = object_files;
	   last_one->next;
	   last_one = last_one->next);
      last_one->next = objfile;
    }

  rt_common_objfile = objfile;
}

/* Read all dynamically loaded common symbol definitions from the inferior
   and put them into the minimal symbol table for the runtime common
   objfile.  */

static void
solib_add_common_symbols (CORE_ADDR rtc_symp)
{
  struct rtc_symb inferior_rtc_symb;
  struct nlist inferior_rtc_nlist;
  int len;
  char *name;

  /* Remove any runtime common symbols from previous runs.  */

  if (rt_common_objfile != NULL && rt_common_objfile->minimal_symbol_count)
    {
      obstack_free (&rt_common_objfile->objfile_obstack, 0);
      obstack_init (&rt_common_objfile->objfile_obstack);
      rt_common_objfile->minimal_symbol_count = 0;
      rt_common_objfile->msymbols = NULL;
      terminate_minimal_symbol_table (rt_common_objfile);
    }

  init_minimal_symbol_collection ();
  make_cleanup_discard_minimal_symbols ();

  while (rtc_symp)
    {
      read_memory (rtc_symp,
		   (char *) &inferior_rtc_symb,
		   sizeof (inferior_rtc_symb));
      read_memory (SOLIB_EXTRACT_ADDRESS (inferior_rtc_symb.rtc_sp),
		   (char *) &inferior_rtc_nlist,
		   sizeof (inferior_rtc_nlist));
      if (inferior_rtc_nlist.n_type == N_COMM)
	{
	  /* FIXME: The length of the symbol name is not available, but in the
	     current implementation the common symbol is allocated immediately
	     behind the name of the symbol.  */
	  len = inferior_rtc_nlist.n_value - inferior_rtc_nlist.n_un.n_strx;

	  name = xmalloc (len);
	  read_memory (SOLIB_EXTRACT_ADDRESS (inferior_rtc_nlist.n_un.n_name),
		       name, len);

	  /* Allocate the runtime common objfile if necessary.  */
	  if (rt_common_objfile == NULL)
	    allocate_rt_common_objfile ();

	  prim_record_minimal_symbol (name, inferior_rtc_nlist.n_value,
				      mst_bss, rt_common_objfile);
	  xfree (name);
	}
      rtc_symp = SOLIB_EXTRACT_ADDRESS (inferior_rtc_symb.rtc_next);
    }

  /* Install any minimal symbols that have been collected as the current
     minimal symbols for the runtime common objfile.  */

  install_minimal_symbols (rt_common_objfile);
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
locate_base (void)
{
  struct minimal_symbol *msymbol;
  CORE_ADDR address = 0;
  char **symbolp;

  /* For SunOS, we want to limit the search for the debug base symbol to the
     executable being debugged, since there is a duplicate named symbol in the
     shared library.  We don't want the shared library versions.  */

  for (symbolp = debug_base_symbols; *symbolp != NULL; symbolp++)
    {
      msymbol = lookup_minimal_symbol (*symbolp, NULL, symfile_objfile);
      if ((msymbol != NULL) && (SYMBOL_VALUE_ADDRESS (msymbol) != 0))
	{
	  address = SYMBOL_VALUE_ADDRESS (msymbol);
	  return (address);
	}
    }
  return (0);
}

/* Locate first member in dynamic linker's map.

   Find the first element in the inferior's dynamic link map, and
   return its address in the inferior.  This function doesn't copy the
   link map entry itself into our address space; current_sos actually
   does the reading.  */

static CORE_ADDR
first_link_map_member (void)
{
  CORE_ADDR lm = 0;

  read_memory (debug_base, (char *) &dynamic_copy, sizeof (dynamic_copy));
  if (dynamic_copy.ld_version >= 2)
    {
      /* It is a version that we can deal with, so read in the secondary
         structure and find the address of the link map list from it.  */
      read_memory (SOLIB_EXTRACT_ADDRESS (dynamic_copy.ld_un.ld_2),
		   (char *) &ld_2_copy, sizeof (struct link_dynamic_2));
      lm = SOLIB_EXTRACT_ADDRESS (ld_2_copy.ld_loaded);
    }
  return (lm);
}

static int
open_symbol_file_object (void *from_ttyp)
{
  return 1;
}


/* Implement the "current_sos" target_so_ops method.  */

static struct so_list *
sunos_current_sos (void)
{
  CORE_ADDR lm;
  struct so_list *head = 0;
  struct so_list **link_ptr = &head;
  int errcode;
  char *buffer;

  /* Make sure we've looked up the inferior's dynamic linker's base
     structure.  */
  if (! debug_base)
    {
      debug_base = locate_base ();

      /* If we can't find the dynamic linker's base structure, this
	 must not be a dynamically linked executable.  Hmm.  */
      if (! debug_base)
	return 0;
    }

  /* Walk the inferior's link map list, and build our list of
     `struct so_list' nodes.  */
  lm = first_link_map_member ();  
  while (lm)
    {
      struct so_list *new
	= (struct so_list *) xmalloc (sizeof (struct so_list));
      struct cleanup *old_chain = make_cleanup (xfree, new);

      memset (new, 0, sizeof (*new));

      new->lm_info = xmalloc (sizeof (struct lm_info));
      make_cleanup (xfree, new->lm_info);

      new->lm_info->lm = xmalloc (sizeof (struct link_map));
      make_cleanup (xfree, new->lm_info->lm);
      memset (new->lm_info->lm, 0, sizeof (struct link_map));

      read_memory (lm, new->lm_info->lm, sizeof (struct link_map));

      lm = lm_next (new);

      /* Extract this shared object's name.  */
      target_read_string (lm_name (new), &buffer,
			  SO_NAME_MAX_PATH_SIZE - 1, &errcode);
      if (errcode != 0)
	warning (_("Can't read pathname for load map: %s."),
		 safe_strerror (errcode));
      else
	{
	  strncpy (new->so_name, buffer, SO_NAME_MAX_PATH_SIZE - 1);
	  new->so_name[SO_NAME_MAX_PATH_SIZE - 1] = '\0';
	  xfree (buffer);
	  strcpy (new->so_original_name, new->so_name);
	}

      /* If this entry has no name, or its name matches the name
	 for the main executable, don't include it in the list.  */
      if (! new->so_name[0]
	  || match_main (new->so_name))
	free_so (new);
      else
	{
	  new->next = 0;
	  *link_ptr = new;
	  link_ptr = &new->next;
	}

      discard_cleanups (old_chain);
    }

  return head;
}


/* On some systems, the only way to recognize the link map entry for
   the main executable file is by looking at its name.  Return
   non-zero iff SONAME matches one of the known main executable names.  */

static int
match_main (char *soname)
{
  char **mainp;

  for (mainp = main_name_list; *mainp != NULL; mainp++)
    {
      if (strcmp (soname, *mainp) == 0)
	return (1);
    }

  return (0);
}


static int
sunos_in_dynsym_resolve_code (CORE_ADDR pc)
{
  return 0;
}

/* Remove the "mapping changed" breakpoint.

   Removes the breakpoint that gets hit when the dynamic linker
   completes a mapping change.  */

static int
disable_break (void)
{
  CORE_ADDR breakpoint_addr;	/* Address where end bkpt is set.  */

  int in_debugger = 0;

  /* Read the debugger structure from the inferior to retrieve the
     address of the breakpoint and the original contents of the
     breakpoint address.  Remove the breakpoint by writing the original
     contents back.  */

  read_memory (debug_addr, (char *) &debug_copy, sizeof (debug_copy));

  /* Set `in_debugger' to zero now.  */

  write_memory (flag_addr, (char *) &in_debugger, sizeof (in_debugger));

  breakpoint_addr = SOLIB_EXTRACT_ADDRESS (debug_copy.ldd_bp_addr);
  write_memory (breakpoint_addr, (char *) &debug_copy.ldd_bp_inst,
		sizeof (debug_copy.ldd_bp_inst));

  /* For the SVR4 version, we always know the breakpoint address.  For the
     SunOS version we don't know it until the above code is executed.
     Grumble if we are stopped anywhere besides the breakpoint address.  */

  if (stop_pc != breakpoint_addr)
    {
      warning (_("stopped at unknown breakpoint "
		 "while handling shared libraries"));
    }

  return 1;
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
   depending upon whether or not the library is being mapped or
   unmapped, and then set to RT_CONSISTENT after the library is
   mapped/unmapped.  */

static int
enable_break (void)
{
  int success = 0;
  int j;
  int in_debugger;

  /* Get link_dynamic structure.  */

  j = target_read_memory (debug_base, (char *) &dynamic_copy,
			  sizeof (dynamic_copy));
  if (j)
    {
      /* unreadable */
      return (0);
    }

  /* Calc address of debugger interface structure.  */

  debug_addr = SOLIB_EXTRACT_ADDRESS (dynamic_copy.ldd);

  /* Calc address of `in_debugger' member of debugger interface structure.  */

  flag_addr = debug_addr + (CORE_ADDR) ((char *) &debug_copy.ldd_in_debugger -
					(char *) &debug_copy);

  /* Write a value of 1 to this member.  */

  in_debugger = 1;
  write_memory (flag_addr, (char *) &in_debugger, sizeof (in_debugger));
  success = 1;

  return (success);
}

/* Implement the "special_symbol_handling" target_so_ops method.

   For SunOS4, this consists of grunging around in the dynamic
   linkers structures to find symbol definitions for "common" symbols
   and adding them to the minimal symbol table for the runtime common
   objfile.  */

static void
sunos_special_symbol_handling (void)
{
  int j;

  if (debug_addr == 0)
    {
      /* Get link_dynamic structure.  */

      j = target_read_memory (debug_base, (char *) &dynamic_copy,
			      sizeof (dynamic_copy));
      if (j)
	{
	  /* unreadable */
	  return;
	}

      /* Calc address of debugger interface structure.  */
      /* FIXME, this needs work for cross-debugging of core files
         (byteorder, size, alignment, etc).  */

      debug_addr = SOLIB_EXTRACT_ADDRESS (dynamic_copy.ldd);
    }

  /* Read the debugger structure from the inferior, just to make sure
     we have a current copy.  */

  j = target_read_memory (debug_addr, (char *) &debug_copy,
			  sizeof (debug_copy));
  if (j)
    return;			/* unreadable */

  /* Get common symbol definitions for the loaded object.  */

  if (debug_copy.ldd_cp)
    {
      solib_add_common_symbols (SOLIB_EXTRACT_ADDRESS (debug_copy.ldd_cp));
    }
}

/* Implement the "create_inferior_hook" target_solib_ops method.

   For SunOS executables, this first instruction is typically the
   one at "_start", or a similar text label, regardless of whether
   the executable is statically or dynamically linked.  The runtime
   startup code takes care of dynamically linking in any shared
   libraries, once gdb allows the inferior to continue.

   We can arrange to cooperate with the dynamic linker to discover the
   names of shared libraries that are dynamically linked, and the base
   addresses to which they are linked.

   This function is responsible for discovering those names and
   addresses, and saving sufficient information about them to allow
   their symbols to be read at a later time.

   FIXME

   Between enable_break() and disable_break(), this code does not
   properly handle hitting breakpoints which the user might have
   set in the startup code or in the dynamic linker itself.  Proper
   handling will probably have to wait until the implementation is
   changed to use the "breakpoint handler function" method.

   Also, what if child has exit()ed?  Must exit loop somehow.  */

static void
sunos_solib_create_inferior_hook (int from_tty)
{
  struct thread_info *tp;
  struct inferior *inf;

  if ((debug_base = locate_base ()) == 0)
    {
      /* Can't find the symbol or the executable is statically linked.  */
      return;
    }

  if (!enable_break ())
    {
      warning (_("shared library handler failed to enable breakpoint"));
      return;
    }

  /* SCO and SunOS need the loop below, other systems should be using the
     special shared library breakpoints and the shared library breakpoint
     service routine.

     Now run the target.  It will eventually hit the breakpoint, at
     which point all of the libraries will have been mapped in and we
     can go groveling around in the dynamic linker structures to find
     out what we need to know about them.  */

  inf = current_inferior ();
  tp = inferior_thread ();

  clear_proceed_status ();

  inf->control.stop_soon = STOP_QUIETLY;
  tp->suspend.stop_signal = GDB_SIGNAL_0;
  do
    {
      target_resume (pid_to_ptid (-1), 0, tp->suspend.stop_signal);
      wait_for_inferior ();
    }
  while (tp->suspend.stop_signal != GDB_SIGNAL_TRAP);
  inf->control.stop_soon = NO_STOP_QUIETLY;

  /* We are now either at the "mapping complete" breakpoint (or somewhere
     else, a condition we aren't prepared to deal with anyway), so adjust
     the PC as necessary after a breakpoint, disable the breakpoint, and
     add any shared libraries that were mapped in.

     Note that adjust_pc_after_break did not perform any PC adjustment,
     as the breakpoint the inferior just hit was not inserted by GDB,
     but by the dynamic loader itself, and is therefore not found on
     the GDB software break point list.  Thus we have to adjust the
     PC here.  */

  if (gdbarch_decr_pc_after_break (target_gdbarch ()))
    {
      stop_pc -= gdbarch_decr_pc_after_break (target_gdbarch ());
      regcache_write_pc (get_current_regcache (), stop_pc);
    }

  if (!disable_break ())
    {
      warning (_("shared library handler failed to disable breakpoint"));
    }

  solib_add ((char *) 0, 0, (struct target_ops *) 0, auto_solib_add);
}

static void
sunos_clear_solib (void)
{
  debug_base = 0;
}

static void
sunos_free_so (struct so_list *so)
{
  xfree (so->lm_info->lm);
  xfree (so->lm_info);
}

static void
sunos_relocate_section_addresses (struct so_list *so,
				  struct target_section *sec)
{
  sec->addr += lm_addr (so);
  sec->endaddr += lm_addr (so);
}

static struct target_so_ops sunos_so_ops;

void
_initialize_sunos_solib (void)
{
  sunos_so_ops.relocate_section_addresses = sunos_relocate_section_addresses;
  sunos_so_ops.free_so = sunos_free_so;
  sunos_so_ops.clear_solib = sunos_clear_solib;
  sunos_so_ops.solib_create_inferior_hook = sunos_solib_create_inferior_hook;
  sunos_so_ops.special_symbol_handling = sunos_special_symbol_handling;
  sunos_so_ops.current_sos = sunos_current_sos;
  sunos_so_ops.open_symbol_file_object = open_symbol_file_object;
  sunos_so_ops.in_dynsym_resolve_code = sunos_in_dynsym_resolve_code;
  sunos_so_ops.bfd_open = solib_bfd_open;

  /* FIXME: Don't do this here.  *_gdbarch_init() should set so_ops.  */
  current_target_so_ops = &sunos_so_ops;
}
