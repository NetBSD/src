/* Handle SunOS and SVR4 shared libraries for GDB, the GNU Debugger.
   Copyright 1990, 91, 92, 93, 94, 95, 96, 98, 1999
   Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */


#include "defs.h"

/* This file is only compilable if link.h is available. */

#ifdef HAVE_LINK_H

#include <sys/types.h>
#include <signal.h>
#include "gdb_string.h"
#include <sys/param.h>
#include <fcntl.h>

#ifndef SVR4_SHARED_LIBS
 /* SunOS shared libs need the nlist structure.  */
#include <a.out.h>
#else
#include "elf/external.h"
#include "elf/common.h"
#endif

#include <link.h>

#include "symtab.h"
#include "bfd.h"
#include "symfile.h"
#include "objfiles.h"
#include "gdbcore.h"
#include "command.h"
#include "target.h"
#include "frame.h"
#include "gdb_regex.h"
#include "inferior.h"
#include "environ.h"
#include "language.h"
#include "gdbcmd.h"

#define MAX_PATH_SIZE 512	/* FIXME: Should be dynamic */

/* On SVR4 systems, a list of symbols in the dynamic linker where
   GDB can try to place a breakpoint to monitor shared library
   events.

   If none of these symbols are found, or other errors occur, then
   SVR4 systems will fall back to using a symbol as the "startup
   mapping complete" breakpoint address.  */

#ifdef SVR4_SHARED_LIBS
static char *solib_break_names[] =
{
  "r_debug_state",
  "_r_debug_state",
  "_dl_debug_state",
  "rtld_db_dlactivity",
  "_rtld_debug_state",
  NULL
};
#endif

#define BKPT_AT_SYMBOL 1

#if defined (BKPT_AT_SYMBOL) && defined (SVR4_SHARED_LIBS)
static char *bkpt_names[] =
{
#ifdef SOLIB_BKPT_NAME
  SOLIB_BKPT_NAME,		/* Prefer configured name if it exists. */
#endif
  "_start",
  "main",
  NULL
};
#endif

/* Symbols which are used to locate the base of the link map structures. */

#ifndef SVR4_SHARED_LIBS
static char *debug_base_symbols[] =
{
  "_DYNAMIC",
  "_DYNAMIC__MGC",
  NULL
};
#endif

static char *main_name_list[] =
{
  "main_$main",
  NULL
};

/* local data declarations */

/* Macro to extract an address from a solib structure.
   When GDB is configured for some 32-bit targets (e.g. Solaris 2.7
   sparc), BFD is configured to handle 64-bit targets, so CORE_ADDR is
   64 bits.  We have to extract only the significant bits of addresses
   to get the right address when accessing the core file BFD.  */

#define SOLIB_EXTRACT_ADDRESS(member) \
  extract_address (&member, sizeof (member))

#ifndef SVR4_SHARED_LIBS

#define LM_ADDR(so) (SOLIB_EXTRACT_ADDRESS ((so) -> lm.lm_addr))
#define LM_NEXT(so) (SOLIB_EXTRACT_ADDRESS ((so) -> lm.lm_next))
#define LM_NAME(so) (SOLIB_EXTRACT_ADDRESS ((so) -> lm.lm_name))
/* Test for first link map entry; first entry is a shared library. */
#define IGNORE_FIRST_LINK_MAP_ENTRY(so) (0)
static struct link_dynamic dynamic_copy;
static struct link_dynamic_2 ld_2_copy;
static struct ld_debug debug_copy;
static CORE_ADDR debug_addr;
static CORE_ADDR flag_addr;

#else /* SVR4_SHARED_LIBS */

#define LM_ADDR(so) (SOLIB_EXTRACT_ADDRESS ((so) -> lm.l_addr))
#define LM_NEXT(so) (SOLIB_EXTRACT_ADDRESS ((so) -> lm.l_next))
#define LM_NAME(so) (SOLIB_EXTRACT_ADDRESS ((so) -> lm.l_name))
#ifdef __mips__
#define LM_OFFS(so) ((so) -> lm.l_offs)
#endif
/* Test for first link map entry; first entry is the exec-file. */
#define IGNORE_FIRST_LINK_MAP_ENTRY(so) \
  (SOLIB_EXTRACT_ADDRESS ((so) -> lm.l_prev) == 0)
static struct r_debug debug_copy;
char shadow_contents[BREAKPOINT_MAX];	/* Stash old bkpt addr contents */

#endif /* !SVR4_SHARED_LIBS */

struct so_list
  {
    /* The following fields of the structure come directly from the
       dynamic linker's tables in the inferior, and are initialized by
       current_sos.  */

    struct so_list *next;	/* next structure in linked list */
    struct link_map lm;		/* copy of link map from inferior */
    CORE_ADDR lmaddr;		/* addr in inferior lm was read from */

    /* Shared object file name, exactly as it appears in the
       inferior's link map.  This may be a relative path, or something
       which needs to be looked up in LD_LIBRARY_PATH, etc.  We use it
       to tell which entries in the inferior's dynamic linker's link
       map we've already loaded.  */
    char so_original_name[MAX_PATH_SIZE];

    /* shared object file name, expanded to something GDB can open */
    char so_name[MAX_PATH_SIZE];

    /* The following fields of the structure are built from
       information gathered from the shared object file itself, and
       are initialized when we actually add it to our symbol tables.  */

    bfd *abfd;
    CORE_ADDR lmend;		/* upper addr bound of mapped object */
    char symbols_loaded;	/* flag: symbols read in yet? */
    char from_tty;		/* flag: print msgs? */
    struct objfile *objfile;	/* objfile for loaded lib */
    struct section_table *sections;
    struct section_table *sections_end;
    struct section_table *textsection;
  };

static struct so_list *so_list_head;	/* List of known shared objects */
static CORE_ADDR debug_base;	/* Base of dynamic linker structures */
static CORE_ADDR breakpoint_addr;	/* Address where end bkpt is set */

static int solib_cleanup_queued = 0;	/* make_run_cleanup called */

extern int
fdmatch PARAMS ((int, int));	/* In libiberty */

/* Local function prototypes */

static void
do_clear_solib PARAMS ((PTR));

static int
match_main PARAMS ((char *));

static void
special_symbol_handling PARAMS ((void));

static void
sharedlibrary_command PARAMS ((char *, int));

static int
enable_break PARAMS ((void));

static void
info_sharedlibrary_command PARAMS ((char *, int));

static int symbol_add_stub PARAMS ((PTR));

static CORE_ADDR
  first_link_map_member PARAMS ((void));

static CORE_ADDR
  locate_base PARAMS ((void));

static int solib_map_sections PARAMS ((PTR));

#ifdef SVR4_SHARED_LIBS

static CORE_ADDR
  elf_locate_base PARAMS ((void));

#else

static struct so_list *current_sos (void);
static void free_so (struct so_list *node);

static int
disable_break PARAMS ((void));

static void
allocate_rt_common_objfile PARAMS ((void));

static void
solib_add_common_symbols (CORE_ADDR);

#endif

void _initialize_solib PARAMS ((void));

/* If non-zero, this is a prefix that will be added to the front of the name
   shared libraries with an absolute filename for loading.  */
static char *solib_absolute_prefix = NULL;

/* If non-empty, this is a search path for loading non-absolute shared library
   symbol files.  This takes precedence over the environment variables PATH
   and LD_LIBRARY_PATH.  */
static char *solib_search_path = NULL;

/*

   LOCAL FUNCTION

   solib_map_sections -- open bfd and build sections for shared lib

   SYNOPSIS

   static int solib_map_sections (struct so_list *so)

   DESCRIPTION

   Given a pointer to one of the shared objects in our list
   of mapped objects, use the recorded name to open a bfd
   descriptor for the object, build a section table, and then
   relocate all the section addresses by the base address at
   which the shared object was mapped.

   FIXMES

   In most (all?) cases the shared object file name recorded in the
   dynamic linkage tables will be a fully qualified pathname.  For
   cases where it isn't, do we really mimic the systems search
   mechanism correctly in the below code (particularly the tilde
   expansion stuff?).
 */

static int
solib_map_sections (arg)
     PTR arg;
{
  struct so_list *so = (struct so_list *) arg;	/* catch_errors bogon */
  char *filename;
  char *scratch_pathname;
  int scratch_chan;
  struct section_table *p;
  struct cleanup *old_chain;
  bfd *abfd;

  filename = tilde_expand (so->so_name);

  if (solib_absolute_prefix && ROOTED_P (filename))
    /* Prefix shared libraries with absolute filenames with
       SOLIB_ABSOLUTE_PREFIX.  */
    {
      char *pfxed_fn;
      int pfx_len;

      pfx_len = strlen (solib_absolute_prefix);

      /* Remove trailing slashes.  */
      while (pfx_len > 0 && SLASH_P (solib_absolute_prefix[pfx_len - 1]))
	pfx_len--;

      pfxed_fn = xmalloc (pfx_len + strlen (filename) + 1);
      strcpy (pfxed_fn, solib_absolute_prefix);
      strcat (pfxed_fn, filename);
      free (filename);

      filename = pfxed_fn;
    }

  old_chain = make_cleanup (free, filename);

  scratch_chan = -1;

  if (solib_search_path)
    scratch_chan = openp (solib_search_path,
			  1, filename, O_RDONLY, 0, &scratch_pathname);
  if (scratch_chan < 0)
    scratch_chan = openp (get_in_environ (inferior_environ, "PATH"),
			  1, filename, O_RDONLY, 0, &scratch_pathname);
  if (scratch_chan < 0)
    {
      scratch_chan = openp (get_in_environ
			    (inferior_environ, "LD_LIBRARY_PATH"),
			    1, filename, O_RDONLY, 0, &scratch_pathname);
    }
  if (scratch_chan < 0)
    {
      perror_with_name (filename);
    }
  /* Leave scratch_pathname allocated.  abfd->name will point to it.  */

  abfd = bfd_fdopenr (scratch_pathname, gnutarget, scratch_chan);
  if (!abfd)
    {
      close (scratch_chan);
      error ("Could not open `%s' as an executable file: %s",
	     scratch_pathname, bfd_errmsg (bfd_get_error ()));
    }
  /* Leave bfd open, core_xfer_memory and "info files" need it.  */
  so->abfd = abfd;
  abfd->cacheable = true;

  /* copy full path name into so_name, so that later symbol_file_add can find
     it */
  if (strlen (scratch_pathname) >= MAX_PATH_SIZE)
    error ("Full path name length of shared library exceeds MAX_PATH_SIZE in so_list structure.");
  strcpy (so->so_name, scratch_pathname);

  if (!bfd_check_format (abfd, bfd_object))
    {
      error ("\"%s\": not in executable format: %s.",
	     scratch_pathname, bfd_errmsg (bfd_get_error ()));
    }
  if (build_section_table (abfd, &so->sections, &so->sections_end))
    {
      error ("Can't find the file sections in `%s': %s",
	     bfd_get_filename (abfd), bfd_errmsg (bfd_get_error ()));
    }

  for (p = so->sections; p < so->sections_end; p++)
    {
      /* Relocate the section binding addresses as recorded in the shared
         object's file by the base address to which the object was actually
         mapped. */
#if defined(__mips__) && defined(__NetBSD__)
      p->addr += LM_OFFS (so);
      p->endaddr += LM_OFFS (so);
#else
      p->addr += LM_ADDR (so);
      p->endaddr += LM_ADDR (so);
#endif
      so->lmend = max (p->endaddr, so->lmend);
      if (STREQ (p->the_bfd_section->name, ".text"))
	{
	  so->textsection = p;
	}
    }

  /* Free the file names, close the file now.  */
  do_cleanups (old_chain);

  return (1);
}

#ifndef SVR4_SHARED_LIBS

/* Allocate the runtime common object file.  */

static void
allocate_rt_common_objfile ()
{
  struct objfile *objfile;
  struct objfile *last_one;

  objfile = (struct objfile *) xmalloc (sizeof (struct objfile));
  memset (objfile, 0, sizeof (struct objfile));
  objfile->md = NULL;
  obstack_specify_allocation (&objfile->psymbol_cache.cache, 0, 0,
			      xmalloc, free);
  obstack_specify_allocation (&objfile->psymbol_obstack, 0, 0, xmalloc,
			      free);
  obstack_specify_allocation (&objfile->symbol_obstack, 0, 0, xmalloc,
			      free);
  obstack_specify_allocation (&objfile->type_obstack, 0, 0, xmalloc,
			      free);
  objfile->name = mstrsave (objfile->md, "rt_common");

  /* Add this file onto the tail of the linked list of other such files. */

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
solib_add_common_symbols (rtc_symp)
     CORE_ADDR rtc_symp;
{
  struct rtc_symb inferior_rtc_symb;
  struct nlist inferior_rtc_nlist;
  int len;
  char *name;

  /* Remove any runtime common symbols from previous runs.  */

  if (rt_common_objfile != NULL && rt_common_objfile->minimal_symbol_count)
    {
      obstack_free (&rt_common_objfile->symbol_obstack, 0);
      obstack_specify_allocation (&rt_common_objfile->symbol_obstack, 0, 0,
				  xmalloc, free);
      rt_common_objfile->minimal_symbol_count = 0;
      rt_common_objfile->msymbols = NULL;
    }

  init_minimal_symbol_collection ();
  make_cleanup ((make_cleanup_func) discard_minimal_symbols, 0);

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
	     behind the name of the symbol. */
	  len = inferior_rtc_nlist.n_value - inferior_rtc_nlist.n_un.n_strx;

	  name = xmalloc (len);
	  read_memory (SOLIB_EXTRACT_ADDRESS (inferior_rtc_nlist.n_un.n_name),
		       name, len);

	  /* Allocate the runtime common objfile if necessary. */
	  if (rt_common_objfile == NULL)
	    allocate_rt_common_objfile ();

	  prim_record_minimal_symbol (name, inferior_rtc_nlist.n_value,
				      mst_bss, rt_common_objfile);
	  free (name);
	}
      rtc_symp = SOLIB_EXTRACT_ADDRESS (inferior_rtc_symb.rtc_next);
    }

  /* Install any minimal symbols that have been collected as the current
     minimal symbols for the runtime common objfile.  */

  install_minimal_symbols (rt_common_objfile);
}

#endif /* SVR4_SHARED_LIBS */


#ifdef SVR4_SHARED_LIBS

static CORE_ADDR
  bfd_lookup_symbol PARAMS ((bfd *, char *));

/*

   LOCAL FUNCTION

   bfd_lookup_symbol -- lookup the value for a specific symbol

   SYNOPSIS

   CORE_ADDR bfd_lookup_symbol (bfd *abfd, char *symname)

   DESCRIPTION

   An expensive way to lookup the value of a single symbol for
   bfd's that are only temporary anyway.  This is used by the
   shared library support to find the address of the debugger
   interface structures in the shared library.

   Note that 0 is specifically allowed as an error return (no
   such symbol).
 */

static CORE_ADDR
bfd_lookup_symbol (abfd, symname)
     bfd *abfd;
     char *symname;
{
  unsigned int storage_needed;
  asymbol *sym;
  asymbol **symbol_table;
  unsigned int number_of_symbols;
  unsigned int i;
  struct cleanup *back_to;
  CORE_ADDR symaddr = 0;

  storage_needed = bfd_get_symtab_upper_bound (abfd);

  if (storage_needed > 0)
    {
      symbol_table = (asymbol **) xmalloc (storage_needed);
      back_to = make_cleanup (free, (PTR) symbol_table);
      number_of_symbols = bfd_canonicalize_symtab (abfd, symbol_table);

      for (i = 0; i < number_of_symbols; i++)
	{
	  sym = *symbol_table++;
	  if (STREQ (sym->name, symname))
	    {
	      /* Bfd symbols are section relative. */
	      symaddr = sym->value + sym->section->vma;
	      break;
	    }
	}
      do_cleanups (back_to);
    }
  return (symaddr);
}

#ifdef HANDLE_SVR4_EXEC_EMULATORS

/*
   Solaris BCP (the part of Solaris which allows it to run SunOS4
   a.out files) throws in another wrinkle. Solaris does not fill
   in the usual a.out link map structures when running BCP programs,
   the only way to get at them is via groping around in the dynamic
   linker.
   The dynamic linker and it's structures are located in the shared
   C library, which gets run as the executable's "interpreter" by
   the kernel.

   Note that we can assume nothing about the process state at the time
   we need to find these structures.  We may be stopped on the first
   instruction of the interpreter (C shared library), the first
   instruction of the executable itself, or somewhere else entirely
   (if we attached to the process for example).
 */

static char *debug_base_symbols[] =
{
  "r_debug",			/* Solaris 2.3 */
  "_r_debug",			/* Solaris 2.1, 2.2 */
  NULL
};

static int
look_for_base PARAMS ((int, CORE_ADDR));

/*

   LOCAL FUNCTION

   look_for_base -- examine file for each mapped address segment

   SYNOPSYS

   static int look_for_base (int fd, CORE_ADDR baseaddr)

   DESCRIPTION

   This function is passed to proc_iterate_over_mappings, which
   causes it to get called once for each mapped address space, with
   an open file descriptor for the file mapped to that space, and the
   base address of that mapped space.

   Our job is to find the debug base symbol in the file that this
   fd is open on, if it exists, and if so, initialize the dynamic
   linker structure base address debug_base.

   Note that this is a computationally expensive proposition, since
   we basically have to open a bfd on every call, so we specifically
   avoid opening the exec file.
 */

static int
look_for_base (fd, baseaddr)
     int fd;
     CORE_ADDR baseaddr;
{
  bfd *interp_bfd;
  CORE_ADDR address = 0;
  char **symbolp;

  /* If the fd is -1, then there is no file that corresponds to this
     mapped memory segment, so skip it.  Also, if the fd corresponds
     to the exec file, skip it as well. */

  if (fd == -1
      || (exec_bfd != NULL
	  && fdmatch (fileno ((FILE *) (exec_bfd->iostream)), fd)))
    {
      return (0);
    }

  /* Try to open whatever random file this fd corresponds to.  Note that
     we have no way currently to find the filename.  Don't gripe about
     any problems we might have, just fail. */

  if ((interp_bfd = bfd_fdopenr ("unnamed", gnutarget, fd)) == NULL)
    {
      return (0);
    }
  if (!bfd_check_format (interp_bfd, bfd_object))
    {
      /* FIXME-leak: on failure, might not free all memory associated with
         interp_bfd.  */
      bfd_close (interp_bfd);
      return (0);
    }

  /* Now try to find our debug base symbol in this file, which we at
     least know to be a valid ELF executable or shared library. */

  for (symbolp = debug_base_symbols; *symbolp != NULL; symbolp++)
    {
      address = bfd_lookup_symbol (interp_bfd, *symbolp);
      if (address != 0)
	{
	  break;
	}
    }
  if (address == 0)
    {
      /* FIXME-leak: on failure, might not free all memory associated with
         interp_bfd.  */
      bfd_close (interp_bfd);
      return (0);
    }

  /* Eureka!  We found the symbol.  But now we may need to relocate it
     by the base address.  If the symbol's value is less than the base
     address of the shared library, then it hasn't yet been relocated
     by the dynamic linker, and we have to do it ourself.  FIXME: Note
     that we make the assumption that the first segment that corresponds
     to the shared library has the base address to which the library
     was relocated. */

  if (address < baseaddr)
    {
      address += baseaddr;
    }
  debug_base = address;
  /* FIXME-leak: on failure, might not free all memory associated with
     interp_bfd.  */
  bfd_close (interp_bfd);
  return (1);
}
#endif /* HANDLE_SVR4_EXEC_EMULATORS */

/*

   LOCAL FUNCTION

   elf_locate_base -- locate the base address of dynamic linker structs
   for SVR4 elf targets.

   SYNOPSIS

   CORE_ADDR elf_locate_base (void)

   DESCRIPTION

   For SVR4 elf targets the address of the dynamic linker's runtime
   structure is contained within the dynamic info section in the
   executable file.  The dynamic section is also mapped into the
   inferior address space.  Because the runtime loader fills in the
   real address before starting the inferior, we have to read in the
   dynamic info section from the inferior address space.
   If there are any errors while trying to find the address, we
   silently return 0, otherwise the found address is returned.

 */

static CORE_ADDR
elf_locate_base ()
{
  sec_ptr dyninfo_sect;
  int dyninfo_sect_size;
  CORE_ADDR dyninfo_addr;
  char *buf;
  char *bufend;

  /* Find the start address of the .dynamic section.  */
  dyninfo_sect = bfd_get_section_by_name (exec_bfd, ".dynamic");
  if (dyninfo_sect == NULL)
    return 0;
  dyninfo_addr = bfd_section_vma (exec_bfd, dyninfo_sect);

  /* Read in .dynamic section, silently ignore errors.  */
  dyninfo_sect_size = bfd_section_size (exec_bfd, dyninfo_sect);
  buf = alloca (dyninfo_sect_size);
  if (target_read_memory (dyninfo_addr, buf, dyninfo_sect_size))
    return 0;

  /* Find the DT_DEBUG entry in the .dynamic section.
     For mips elf we look for DT_MIPS_RLD_MAP, mips elf apparently has
     no DT_DEBUG entries.  */
#ifndef TARGET_ELF64
  for (bufend = buf + dyninfo_sect_size;
       buf < bufend;
       buf += sizeof (Elf32_External_Dyn))
    {
      Elf32_External_Dyn *x_dynp = (Elf32_External_Dyn *) buf;
      long dyn_tag;
      CORE_ADDR dyn_ptr;

      dyn_tag = bfd_h_get_32 (exec_bfd, (bfd_byte *) x_dynp->d_tag);
      if (dyn_tag == DT_NULL)
	break;
      else if (dyn_tag == DT_DEBUG)
	{
	  dyn_ptr = bfd_h_get_32 (exec_bfd, (bfd_byte *) x_dynp->d_un.d_ptr);
	  return dyn_ptr;
	}
#ifdef DT_MIPS_RLD_MAP
      else if (dyn_tag == DT_MIPS_RLD_MAP)
	{
	  char pbuf[TARGET_PTR_BIT / HOST_CHAR_BIT];

	  /* DT_MIPS_RLD_MAP contains a pointer to the address
	     of the dynamic link structure.  */
	  dyn_ptr = bfd_h_get_32 (exec_bfd, (bfd_byte *) x_dynp->d_un.d_ptr);
	  if (target_read_memory (dyn_ptr, pbuf, sizeof (pbuf)))
	    return 0;
	  return extract_unsigned_integer (pbuf, sizeof (pbuf));
	}
#endif
    }
#else /* ELF64 */
  for (bufend = buf + dyninfo_sect_size;
       buf < bufend;
       buf += sizeof (Elf64_External_Dyn))
    {
      Elf64_External_Dyn *x_dynp = (Elf64_External_Dyn *) buf;
      long dyn_tag;
      CORE_ADDR dyn_ptr;

      dyn_tag = bfd_h_get_64 (exec_bfd, (bfd_byte *) x_dynp->d_tag);
      if (dyn_tag == DT_NULL)
	break;
      else if (dyn_tag == DT_DEBUG)
	{
	  dyn_ptr = bfd_h_get_64 (exec_bfd, (bfd_byte *) x_dynp->d_un.d_ptr);
	  return dyn_ptr;
	}
    }
#endif

  /* DT_DEBUG entry not found.  */
  return 0;
}

#endif /* SVR4_SHARED_LIBS */

/*

   LOCAL FUNCTION

   locate_base -- locate the base address of dynamic linker structs

   SYNOPSIS

   CORE_ADDR locate_base (void)

   DESCRIPTION

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
   executable symbol tables.

 */

static CORE_ADDR
locate_base ()
{

#ifndef SVR4_SHARED_LIBS

  struct minimal_symbol *msymbol;
  CORE_ADDR address = 0;
  char **symbolp;

  /* For SunOS, we want to limit the search for the debug base symbol to the
     executable being debugged, since there is a duplicate named symbol in the
     shared library.  We don't want the shared library versions. */

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

#else /* SVR4_SHARED_LIBS */

  /* Check to see if we have a currently valid address, and if so, avoid
     doing all this work again and just return the cached address.  If
     we have no cached address, try to locate it in the dynamic info
     section for ELF executables.  */

  if (debug_base == 0)
    {
      if (exec_bfd != NULL
	  && bfd_get_flavour (exec_bfd) == bfd_target_elf_flavour)
	debug_base = elf_locate_base ();
#ifdef HANDLE_SVR4_EXEC_EMULATORS
      /* Try it the hard way for emulated executables.  */
      else if (inferior_pid != 0 && target_has_execution)
	proc_iterate_over_mappings (look_for_base);
#endif
    }
  return (debug_base);

#endif /* !SVR4_SHARED_LIBS */

}

/*

   LOCAL FUNCTION

   first_link_map_member -- locate first member in dynamic linker's map

   SYNOPSIS

   static CORE_ADDR first_link_map_member (void)

   DESCRIPTION

   Find the first element in the inferior's dynamic link map, and
   return its address in the inferior.  This function doesn't copy the
   link map entry itself into our address space; current_sos actually
   does the reading.  */

static CORE_ADDR
first_link_map_member ()
{
  CORE_ADDR lm = 0;

#ifndef SVR4_SHARED_LIBS

  read_memory (debug_base, (char *) &dynamic_copy, sizeof (dynamic_copy));
  if (dynamic_copy.ld_version >= 2)
    {
      /* It is a version that we can deal with, so read in the secondary
         structure and find the address of the link map list from it. */
      read_memory (SOLIB_EXTRACT_ADDRESS (dynamic_copy.ld_un.ld_2),
		   (char *) &ld_2_copy, sizeof (struct link_dynamic_2));
      lm = SOLIB_EXTRACT_ADDRESS (ld_2_copy.ld_loaded);
    }

#else /* SVR4_SHARED_LIBS */

  read_memory (debug_base, (char *) &debug_copy, sizeof (struct r_debug));
  /* FIXME:  Perhaps we should validate the info somehow, perhaps by
     checking r_version for a known version number, or r_state for
     RT_CONSISTENT. */
  lm = SOLIB_EXTRACT_ADDRESS (debug_copy.r_map);

#endif /* !SVR4_SHARED_LIBS */

  return (lm);
}

#ifdef SVR4_SHARED_LIBS
/*

  LOCAL FUNCTION

  open_symbol_file_object

  SYNOPSIS

  void open_symbol_file_object (int from_tty)

  DESCRIPTION

  If no open symbol file, attempt to locate and open the main symbol
  file.  On SVR4 systems, this is the first link map entry.  If its
  name is here, we can open it.  Useful when attaching to a process
  without first loading its symbol file.

 */

static int
open_symbol_file_object (from_ttyp)
     int *from_ttyp;	/* sneak past catch_errors */
{
  CORE_ADDR lm;
  struct link_map lmcopy;
  char *filename;
  int errcode;

  if (symfile_objfile)
    if (!query ("Attempt to reload symbols from process? "))
      return 0;

  if ((debug_base = locate_base ()) == 0)
    return 0;	/* failed somehow... */

  /* First link map member should be the executable.  */
  if ((lm = first_link_map_member ()) == 0)
    return 0;	/* failed somehow... */

  /* Read from target memory to GDB.  */
  read_memory (lm, (void *) &lmcopy, sizeof (lmcopy));

  if (lmcopy.l_name == 0)
    return 0;	/* no filename.  */

  /* Now fetch the filename from target memory.  */
  target_read_string (SOLIB_EXTRACT_ADDRESS (lmcopy.l_name), &filename, 
		      MAX_PATH_SIZE - 1, &errcode);
  if (errcode)
    {
      warning ("failed to read exec filename from attached file: %s",
	       safe_strerror (errcode));
      return 0;
    }

  make_cleanup ((make_cleanup_func) free, (void *) filename);
  /* Have a pathname: read the symbol file.  */
  symbol_file_command (filename, *from_ttyp);

  return 1;
}
#endif /* SVR4_SHARED_LIBS */


/* LOCAL FUNCTION

   free_so --- free a `struct so_list' object

   SYNOPSIS

   void free_so (struct so_list *so)

   DESCRIPTION

   Free the storage associated with the `struct so_list' object SO.
   If we have opened a BFD for SO, close it.  

   The caller is responsible for removing SO from whatever list it is
   a member of.  If we have placed SO's sections in some target's
   section table, the caller is responsible for removing them.

   This function doesn't mess with objfiles at all.  If there is an
   objfile associated with SO that needs to be removed, the caller is
   responsible for taking care of that.  */

static void
free_so (struct so_list *so)
{
  char *bfd_filename = 0;

  if (so->sections)
    free (so->sections);
      
  if (so->abfd)
    {
      bfd_filename = bfd_get_filename (so->abfd);
      if (! bfd_close (so->abfd))
	warning ("cannot close \"%s\": %s",
		 bfd_filename, bfd_errmsg (bfd_get_error ()));
    }

  if (bfd_filename)
    free (bfd_filename);

  free (so);
}


/* On some systems, the only way to recognize the link map entry for
   the main executable file is by looking at its name.  Return
   non-zero iff SONAME matches one of the known main executable names.  */

static int
match_main (soname)
     char *soname;
{
  char **mainp;

  for (mainp = main_name_list; *mainp != NULL; mainp++)
    {
      if (strcmp (soname, *mainp) == 0)
	return (1);
    }

  return (0);
}


/* LOCAL FUNCTION

   current_sos -- build a list of currently loaded shared objects

   SYNOPSIS

   struct so_list *current_sos ()

   DESCRIPTION

   Build a list of `struct so_list' objects describing the shared
   objects currently loaded in the inferior.  This list does not
   include an entry for the main executable file.

   Note that we only gather information directly available from the
   inferior --- we don't examine any of the shared library files
   themselves.  The declaration of `struct so_list' says which fields
   we provide values for.  */

static struct so_list *
current_sos ()
{
  CORE_ADDR lm;
  struct so_list *head = 0;
  struct so_list **link_ptr = &head;

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
      struct cleanup *old_chain = make_cleanup (free, new);
      memset (new, 0, sizeof (*new));

      new->lmaddr = lm;
      read_memory (lm, (char *) &(new->lm), sizeof (struct link_map));

      lm = LM_NEXT (new);

      /* For SVR4 versions, the first entry in the link map is for the
         inferior executable, so we must ignore it.  For some versions of
         SVR4, it has no name.  For others (Solaris 2.3 for example), it
         does have a name, so we can no longer use a missing name to
         decide when to ignore it. */
      if (IGNORE_FIRST_LINK_MAP_ENTRY (new))
	free_so (new);
      else
	{
	  int errcode;
	  char *buffer;

	  /* Extract this shared object's name.  */
	  target_read_string (LM_NAME (new), &buffer,
			      MAX_PATH_SIZE - 1, &errcode);
	  if (errcode != 0)
	    {
	      warning ("current_sos: Can't read pathname for load map: %s\n",
		       safe_strerror (errcode));
	    }
	  else
	    {
	      strncpy (new->so_name, buffer, MAX_PATH_SIZE - 1);
	      new->so_name[MAX_PATH_SIZE - 1] = '\0';
	      free (buffer);
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
	}

      discard_cleanups (old_chain);
    }

  return head;
}


/* A small stub to get us past the arg-passing pinhole of catch_errors.  */

static int
symbol_add_stub (arg)
     PTR arg;
{
  register struct so_list *so = (struct so_list *) arg;  /* catch_errs bogon */
  CORE_ADDR text_addr = 0;
  struct section_addr_info *sap;

  /* Have we already loaded this shared object?  */
  ALL_OBJFILES (so->objfile)
    {
      if (strcmp (so->objfile->name, so->so_name) == 0)
	return 1;
    }

  /* Find the shared object's text segment.  */
  if (so->textsection)
    text_addr = so->textsection->addr;
  else if (so->abfd != NULL)
    {
      asection *lowest_sect;

      /* If we didn't find a mapped non zero sized .text section, set up
         text_addr so that the relocation in symbol_file_add does no harm.  */
      lowest_sect = bfd_get_section_by_name (so->abfd, ".text");
      if (lowest_sect == NULL)
	bfd_map_over_sections (so->abfd, find_lowest_section,
			       (PTR) &lowest_sect);
      if (lowest_sect)
	text_addr = bfd_section_vma (so->abfd, lowest_sect)
	  + LM_ADDR (so);
    }

  sap = build_section_addr_info_from_section_table (so->sections,
                                                    so->sections_end);
  sap->text_addr = text_addr;
  so->objfile = symbol_file_add (so->so_name, so->from_tty,
				 sap, 0, OBJF_SHARED);
  free_section_addr_info (sap);

  return (1);
}


/* LOCAL FUNCTION

   update_solib_list --- synchronize GDB's shared object list with inferior's

   SYNOPSIS

   void update_solib_list (int from_tty, struct target_ops *TARGET)

   Extract the list of currently loaded shared objects from the
   inferior, and compare it with the list of shared objects currently
   in GDB's so_list_head list.  Edit so_list_head to bring it in sync
   with the inferior's new list.

   If we notice that the inferior has unloaded some shared objects,
   free any symbolic info GDB had read about those shared objects.

   Don't load symbolic info for any new shared objects; just add them
   to the list, and leave their symbols_loaded flag clear.

   If FROM_TTY is non-null, feel free to print messages about what
   we're doing.

   If TARGET is non-null, add the sections of all new shared objects
   to TARGET's section table.  Note that this doesn't remove any
   sections for shared objects that have been unloaded, and it
   doesn't check to see if the new shared objects are already present in
   the section table.  But we only use this for core files and
   processes we've just attached to, so that's okay.  */

void
update_solib_list (int from_tty, struct target_ops *target)
{
  struct so_list *inferior = current_sos ();
  struct so_list *gdb, **gdb_link;

#ifdef SVR4_SHARED_LIBS
  /* If we are attaching to a running process for which we 
     have not opened a symbol file, we may be able to get its 
     symbols now!  */
  if (attach_flag &&
      symfile_objfile == NULL)
    catch_errors (open_symbol_file_object, (PTR) &from_tty, 
		  "Error reading attached process's symbol file.\n",
		  RETURN_MASK_ALL);

#endif SVR4_SHARED_LIBS

  /* Since this function might actually add some elements to the
     so_list_head list, arrange for it to be cleaned up when
     appropriate.  */
  if (!solib_cleanup_queued)
    {
      make_run_cleanup (do_clear_solib, NULL);
      solib_cleanup_queued = 1;
    }

  /* GDB and the inferior's dynamic linker each maintain their own
     list of currently loaded shared objects; we want to bring the
     former in sync with the latter.  Scan both lists, seeing which
     shared objects appear where.  There are three cases:

     - A shared object appears on both lists.  This means that GDB
     knows about it already, and it's still loaded in the inferior.
     Nothing needs to happen.

     - A shared object appears only on GDB's list.  This means that
     the inferior has unloaded it.  We should remove the shared
     object from GDB's tables.

     - A shared object appears only on the inferior's list.  This
     means that it's just been loaded.  We should add it to GDB's
     tables.

     So we walk GDB's list, checking each entry to see if it appears
     in the inferior's list too.  If it does, no action is needed, and
     we remove it from the inferior's list.  If it doesn't, the
     inferior has unloaded it, and we remove it from GDB's list.  By
     the time we're done walking GDB's list, the inferior's list
     contains only the new shared objects, which we then add.  */

  gdb = so_list_head;
  gdb_link = &so_list_head;
  while (gdb)
    {
      struct so_list *i = inferior;
      struct so_list **i_link = &inferior;

      /* Check to see whether the shared object *gdb also appears in
	 the inferior's current list.  */
      while (i)
	{
	  if (! strcmp (gdb->so_original_name, i->so_original_name))
	    break;

	  i_link = &i->next;
	  i = *i_link;
	}

      /* If the shared object appears on the inferior's list too, then
         it's still loaded, so we don't need to do anything.  Delete
         it from the inferior's list, and leave it on GDB's list.  */
      if (i)
	{
	  *i_link = i->next;
	  free_so (i);
	  gdb_link = &gdb->next;
	  gdb = *gdb_link;
	}

      /* If it's not on the inferior's list, remove it from GDB's tables.  */
      else
	{
	  *gdb_link = gdb->next;

	  /* Unless the user loaded it explicitly, free SO's objfile.  */
	  if (gdb->objfile && ! (gdb->objfile->flags & OBJF_USERLOADED))
	    free_objfile (gdb->objfile);

	  /* Some targets' section tables might be referring to
	     sections from so->abfd; remove them.  */
	  remove_target_sections (gdb->abfd);

	  free_so (gdb);
	  gdb = *gdb_link;
	}
    }

  /* Now the inferior's list contains only shared objects that don't
     appear in GDB's list --- those that are newly loaded.  Add them
     to GDB's shared object list.  */
  if (inferior)
    {
      struct so_list *i;

      /* Add the new shared objects to GDB's list.  */
      *gdb_link = inferior;

      /* Fill in the rest of each of the `struct so_list' nodes.  */
      for (i = inferior; i; i = i->next)
	{
	  i->from_tty = from_tty;

	  /* Fill in the rest of the `struct so_list' node.  */
	  catch_errors (solib_map_sections, i,
			"Error while mapping shared library sections:\n",
			RETURN_MASK_ALL);
	}

      /* If requested, add the shared objects' sections to the the
	 TARGET's section table.  */
      if (target)
	{
	  int new_sections;

	  /* Figure out how many sections we'll need to add in total.  */
	  new_sections = 0;
	  for (i = inferior; i; i = i->next)
	    new_sections += (i->sections_end - i->sections);

	  if (new_sections > 0)
	    {
	      int space = target_resize_to_sections (target, new_sections);

	      for (i = inferior; i; i = i->next)
		{
		  int count = (i->sections_end - i->sections);
		  memcpy (target->to_sections + space,
			  i->sections,
			  count * sizeof (i->sections[0]));
		  space += count;
		}
	    }
	}
    }
}


/* GLOBAL FUNCTION

   solib_add -- read in symbol info for newly added shared libraries

   SYNOPSIS

   void solib_add (char *pattern, int from_tty, struct target_ops *TARGET)

   DESCRIPTION

   Read in symbolic information for any shared objects whose names
   match PATTERN.  (If we've already read a shared object's symbol
   info, leave it alone.)  If PATTERN is zero, read them all.

   FROM_TTY and TARGET are as described for update_solib_list, above.  */

void
solib_add (char *pattern, int from_tty, struct target_ops *target)
{
  struct so_list *gdb;

  if (pattern)
    {
      char *re_err = re_comp (pattern);

      if (re_err)
	error ("Invalid regexp: %s", re_err);
    }

  update_solib_list (from_tty, target);

  /* Walk the list of currently loaded shared libraries, and read
     symbols for any that match the pattern --- or any whose symbols
     aren't already loaded, if no pattern was given.  */
  {
    int any_matches = 0;
    int loaded_any_symbols = 0;

    for (gdb = so_list_head; gdb; gdb = gdb->next)
      if (! pattern || re_exec (gdb->so_name))
	{
	  any_matches = 1;

	  if (gdb->symbols_loaded)
	    {
	      if (from_tty)
		printf_unfiltered ("Symbols already loaded for %s\n",
				   gdb->so_name);
	    }
	  else
	    {
	      if (catch_errors
		  (symbol_add_stub, gdb,
		   "Error while reading shared library symbols:\n",
		   RETURN_MASK_ALL))
		{
		  if (from_tty)
		    printf_unfiltered ("Loaded symbols for %s\n",
				       gdb->so_name);
		  gdb->symbols_loaded = 1;
		  loaded_any_symbols = 1;
		}
	    }
	}

    if (from_tty && pattern && ! any_matches)
      printf_unfiltered
	("No loaded shared libraries match the pattern `%s'.\n", pattern);

    if (loaded_any_symbols)
      {
	/* Getting new symbols may change our opinion about what is
	   frameless.  */
	reinit_frame_cache ();

	special_symbol_handling ();
      }
  }
}


/*

   LOCAL FUNCTION

   info_sharedlibrary_command -- code for "info sharedlibrary"

   SYNOPSIS

   static void info_sharedlibrary_command ()

   DESCRIPTION

   Walk through the shared library list and print information
   about each attached library.
 */

static void
info_sharedlibrary_command (ignore, from_tty)
     char *ignore;
     int from_tty;
{
  register struct so_list *so = NULL;	/* link map state variable */
  int header_done = 0;
  int addr_width;
  char *addr_fmt;

  if (exec_bfd == NULL)
    {
      printf_unfiltered ("No executable file.\n");
      return;
    }

#ifndef TARGET_ELF64
  addr_width = 8 + 4;
  addr_fmt = "08l";
#else
  addr_width = 16 + 4;
  addr_fmt = "016l";
#endif

  update_solib_list (from_tty, 0);

  for (so = so_list_head; so; so = so->next)
    {
      if (so->so_name[0])
	{
	  if (!header_done)
	    {
	      printf_unfiltered ("%-*s%-*s%-12s%s\n", addr_width, "From",
				 addr_width, "To", "Syms Read",
				 "Shared Object Library");
	      header_done++;
	    }

	  printf_unfiltered ("%-*s", addr_width,
		      local_hex_string_custom ((unsigned long) LM_ADDR (so),
					       addr_fmt));
	  printf_unfiltered ("%-*s", addr_width,
			 local_hex_string_custom ((unsigned long) so->lmend,
						  addr_fmt));
	  printf_unfiltered ("%-12s", so->symbols_loaded ? "Yes" : "No");
	  printf_unfiltered ("%s\n", so->so_name);
	}
    }
  if (so_list_head == NULL)
    {
      printf_unfiltered ("No shared libraries loaded at this time.\n");
    }
}

/*

   GLOBAL FUNCTION

   solib_address -- check to see if an address is in a shared lib

   SYNOPSIS

   char * solib_address (CORE_ADDR address)

   DESCRIPTION

   Provides a hook for other gdb routines to discover whether or
   not a particular address is within the mapped address space of
   a shared library.  Any address between the base mapping address
   and the first address beyond the end of the last mapping, is
   considered to be within the shared library address space, for
   our purposes.

   For example, this routine is called at one point to disable
   breakpoints which are in shared libraries that are not currently
   mapped in.
 */

char *
solib_address (address)
     CORE_ADDR address;
{
  register struct so_list *so = 0;	/* link map state variable */

  for (so = so_list_head; so; so = so->next)
    {
      if (LM_ADDR (so) <= address && address < so->lmend)
	return (so->so_name);
    }

  return (0);
}

/* Called by free_all_symtabs */

void
clear_solib ()
{
  /* This function is expected to handle ELF shared libraries.  It is
     also used on Solaris, which can run either ELF or a.out binaries
     (for compatibility with SunOS 4), both of which can use shared
     libraries.  So we don't know whether we have an ELF executable or
     an a.out executable until the user chooses an executable file.

     ELF shared libraries don't get mapped into the address space
     until after the program starts, so we'd better not try to insert
     breakpoints in them immediately.  We have to wait until the
     dynamic linker has loaded them; we'll hit a bp_shlib_event
     breakpoint (look for calls to create_solib_event_breakpoint) when
     it's ready.

     SunOS shared libraries seem to be different --- they're present
     as soon as the process begins execution, so there's no need to
     put off inserting breakpoints.  There's also nowhere to put a
     bp_shlib_event breakpoint, so if we put it off, we'll never get
     around to it.

     So: disable breakpoints only if we're using ELF shared libs.  */
  if (exec_bfd != NULL
      && bfd_get_flavour (exec_bfd) != bfd_target_aout_flavour)
    disable_breakpoints_in_shlibs (1);

  while (so_list_head)
    {
      struct so_list *so = so_list_head;
      so_list_head = so->next;
      free_so (so);
    }

  debug_base = 0;
}

static void
do_clear_solib (dummy)
     PTR dummy;
{
  solib_cleanup_queued = 0;
  clear_solib ();
}

#ifdef SVR4_SHARED_LIBS

/* Return 1 if PC lies in the dynamic symbol resolution code of the
   SVR4 run time loader.  */

static CORE_ADDR interp_text_sect_low;
static CORE_ADDR interp_text_sect_high;
static CORE_ADDR interp_plt_sect_low;
static CORE_ADDR interp_plt_sect_high;

int
in_svr4_dynsym_resolve_code (pc)
     CORE_ADDR pc;
{
  return ((pc >= interp_text_sect_low && pc < interp_text_sect_high)
	  || (pc >= interp_plt_sect_low && pc < interp_plt_sect_high)
	  || in_plt_section (pc, NULL));
}
#endif

/*

   LOCAL FUNCTION

   disable_break -- remove the "mapping changed" breakpoint

   SYNOPSIS

   static int disable_break ()

   DESCRIPTION

   Removes the breakpoint that gets hit when the dynamic linker
   completes a mapping change.

 */

#ifndef SVR4_SHARED_LIBS

static int
disable_break ()
{
  int status = 1;

#ifndef SVR4_SHARED_LIBS

  int in_debugger = 0;

  /* Read the debugger structure from the inferior to retrieve the
     address of the breakpoint and the original contents of the
     breakpoint address.  Remove the breakpoint by writing the original
     contents back. */

  read_memory (debug_addr, (char *) &debug_copy, sizeof (debug_copy));

  /* Set `in_debugger' to zero now. */

  write_memory (flag_addr, (char *) &in_debugger, sizeof (in_debugger));

  breakpoint_addr = SOLIB_EXTRACT_ADDRESS (debug_copy.ldd_bp_addr);
  write_memory (breakpoint_addr, (char *) &debug_copy.ldd_bp_inst,
		sizeof (debug_copy.ldd_bp_inst));

#else /* SVR4_SHARED_LIBS */

  /* Note that breakpoint address and original contents are in our address
     space, so we just need to write the original contents back. */

  if (memory_remove_breakpoint (breakpoint_addr, shadow_contents) != 0)
    {
      status = 0;
    }

#endif /* !SVR4_SHARED_LIBS */

  /* For the SVR4 version, we always know the breakpoint address.  For the
     SunOS version we don't know it until the above code is executed.
     Grumble if we are stopped anywhere besides the breakpoint address. */

  if (stop_pc != breakpoint_addr)
    {
      warning ("stopped at unknown breakpoint while handling shared libraries");
    }

  return (status);
}

#endif /* #ifdef SVR4_SHARED_LIBS */

/*

   LOCAL FUNCTION

   enable_break -- arrange for dynamic linker to hit breakpoint

   SYNOPSIS

   int enable_break (void)

   DESCRIPTION

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
   and then set to RT_CONSISTENT after the library is mapped/unmapped.
 */

static int
enable_break ()
{
  int success = 0;

#ifndef SVR4_SHARED_LIBS

  int j;
  int in_debugger;

  /* Get link_dynamic structure */

  j = target_read_memory (debug_base, (char *) &dynamic_copy,
			  sizeof (dynamic_copy));
  if (j)
    {
      /* unreadable */
      return (0);
    }

  /* Calc address of debugger interface structure */

  debug_addr = SOLIB_EXTRACT_ADDRESS (dynamic_copy.ldd);

  /* Calc address of `in_debugger' member of debugger interface structure */

  flag_addr = debug_addr + (CORE_ADDR) ((char *) &debug_copy.ldd_in_debugger -
					(char *) &debug_copy);

  /* Write a value of 1 to this member.  */

  in_debugger = 1;
  write_memory (flag_addr, (char *) &in_debugger, sizeof (in_debugger));
  success = 1;

#else /* SVR4_SHARED_LIBS */

#ifdef BKPT_AT_SYMBOL

  struct minimal_symbol *msymbol;
  char **bkpt_namep;
  asection *interp_sect;

  /* First, remove all the solib event breakpoints.  Their addresses
     may have changed since the last time we ran the program.  */
  remove_solib_event_breakpoints ();

#ifdef SVR4_SHARED_LIBS
  interp_text_sect_low = interp_text_sect_high = 0;
  interp_plt_sect_low = interp_plt_sect_high = 0;

  /* Find the .interp section; if not found, warn the user and drop
     into the old breakpoint at symbol code.  */
  interp_sect = bfd_get_section_by_name (exec_bfd, ".interp");
  if (interp_sect)
    {
      unsigned int interp_sect_size;
      char *buf;
      CORE_ADDR load_addr;
      bfd *tmp_bfd;
      CORE_ADDR sym_addr = 0;

      /* Read the contents of the .interp section into a local buffer;
         the contents specify the dynamic linker this program uses.  */
      interp_sect_size = bfd_section_size (exec_bfd, interp_sect);
      buf = alloca (interp_sect_size);
      bfd_get_section_contents (exec_bfd, interp_sect,
				buf, 0, interp_sect_size);

      /* Now we need to figure out where the dynamic linker was
         loaded so that we can load its symbols and place a breakpoint
         in the dynamic linker itself.

         This address is stored on the stack.  However, I've been unable
         to find any magic formula to find it for Solaris (appears to
         be trivial on GNU/Linux).  Therefore, we have to try an alternate
         mechanism to find the dynamic linker's base address.  */
      tmp_bfd = bfd_openr (buf, gnutarget);
      if (tmp_bfd == NULL)
	goto bkpt_at_symbol;

      /* Make sure the dynamic linker's really a useful object.  */
      if (!bfd_check_format (tmp_bfd, bfd_object))
	{
	  warning ("Unable to grok dynamic linker %s as an object file", buf);
	  bfd_close (tmp_bfd);
	  goto bkpt_at_symbol;
	}

      /* We find the dynamic linker's base address by examining the
         current pc (which point at the entry point for the dynamic
         linker) and subtracting the offset of the entry point.  */
      load_addr = read_pc () - tmp_bfd->start_address;

      /* Record the relocated start and end address of the dynamic linker
         text and plt section for in_svr4_dynsym_resolve_code.  */
      interp_sect = bfd_get_section_by_name (tmp_bfd, ".text");
      if (interp_sect)
	{
	  interp_text_sect_low =
	    bfd_section_vma (tmp_bfd, interp_sect) + load_addr;
	  interp_text_sect_high =
	    interp_text_sect_low + bfd_section_size (tmp_bfd, interp_sect);
	}
      interp_sect = bfd_get_section_by_name (tmp_bfd, ".plt");
      if (interp_sect)
	{
	  interp_plt_sect_low =
	    bfd_section_vma (tmp_bfd, interp_sect) + load_addr;
	  interp_plt_sect_high =
	    interp_plt_sect_low + bfd_section_size (tmp_bfd, interp_sect);
	}

      /* Now try to set a breakpoint in the dynamic linker.  */
      for (bkpt_namep = solib_break_names; *bkpt_namep != NULL; bkpt_namep++)
	{
	  sym_addr = bfd_lookup_symbol (tmp_bfd, *bkpt_namep);
	  if (sym_addr != 0)
	    break;
	}

      /* We're done with the temporary bfd.  */
      bfd_close (tmp_bfd);

      if (sym_addr != 0)
	{
	  create_solib_event_breakpoint (load_addr + sym_addr);
	  return 1;
	}

      /* For whatever reason we couldn't set a breakpoint in the dynamic
         linker.  Warn and drop into the old code.  */
    bkpt_at_symbol:
      warning ("Unable to find dynamic linker breakpoint function.\nGDB will be unable to debug shared library initializers\nand track explicitly loaded dynamic code.");
    }
#endif

  /* Scan through the list of symbols, trying to look up the symbol and
     set a breakpoint there.  Terminate loop when we/if we succeed. */

  breakpoint_addr = 0;
  for (bkpt_namep = bkpt_names; *bkpt_namep != NULL; bkpt_namep++)
    {
      msymbol = lookup_minimal_symbol (*bkpt_namep, NULL, symfile_objfile);
      if ((msymbol != NULL) && (SYMBOL_VALUE_ADDRESS (msymbol) != 0))
	{
	  create_solib_event_breakpoint (SYMBOL_VALUE_ADDRESS (msymbol));
	  return 1;
	}
    }

  /* Nothing good happened.  */
  success = 0;

#endif /* BKPT_AT_SYMBOL */

#endif /* !SVR4_SHARED_LIBS */

  return (success);
}

/*

   GLOBAL FUNCTION

   solib_create_inferior_hook -- shared library startup support

   SYNOPSIS

   void solib_create_inferior_hook()

   DESCRIPTION

   When gdb starts up the inferior, it nurses it along (through the
   shell) until it is ready to execute it's first instruction.  At this
   point, this function gets called via expansion of the macro
   SOLIB_CREATE_INFERIOR_HOOK.

   For SunOS executables, this first instruction is typically the
   one at "_start", or a similar text label, regardless of whether
   the executable is statically or dynamically linked.  The runtime
   startup code takes care of dynamically linking in any shared
   libraries, once gdb allows the inferior to continue.

   For SVR4 executables, this first instruction is either the first
   instruction in the dynamic linker (for dynamically linked
   executables) or the instruction at "start" for statically linked
   executables.  For dynamically linked executables, the system
   first exec's /lib/libc.so.N, which contains the dynamic linker,
   and starts it running.  The dynamic linker maps in any needed
   shared libraries, maps in the actual user executable, and then
   jumps to "start" in the user executable.

   For both SunOS shared libraries, and SVR4 shared libraries, we
   can arrange to cooperate with the dynamic linker to discover the
   names of shared libraries that are dynamically linked, and the
   base addresses to which they are linked.

   This function is responsible for discovering those names and
   addresses, and saving sufficient information about them to allow
   their symbols to be read at a later time.

   FIXME

   Between enable_break() and disable_break(), this code does not
   properly handle hitting breakpoints which the user might have
   set in the startup code or in the dynamic linker itself.  Proper
   handling will probably have to wait until the implementation is
   changed to use the "breakpoint handler function" method.

   Also, what if child has exit()ed?  Must exit loop somehow.
 */

void
solib_create_inferior_hook ()
{
  /* If we are using the BKPT_AT_SYMBOL code, then we don't need the base
     yet.  In fact, in the case of a SunOS4 executable being run on
     Solaris, we can't get it yet.  current_sos will get it when it needs
     it.  */
#if !(defined (SVR4_SHARED_LIBS) && defined (BKPT_AT_SYMBOL))
  if ((debug_base = locate_base ()) == 0)
    {
      /* Can't find the symbol or the executable is statically linked. */
      return;
    }
#endif

  if (!enable_break ())
    {
      warning ("shared library handler failed to enable breakpoint");
      return;
    }

#if !defined(SVR4_SHARED_LIBS) || defined(_SCO_DS)
  /* SCO and SunOS need the loop below, other systems should be using the
     special shared library breakpoints and the shared library breakpoint
     service routine.

     Now run the target.  It will eventually hit the breakpoint, at
     which point all of the libraries will have been mapped in and we
     can go groveling around in the dynamic linker structures to find
     out what we need to know about them. */

  clear_proceed_status ();
  stop_soon_quietly = 1;
  stop_signal = TARGET_SIGNAL_0;
  do
    {
      target_resume (-1, 0, stop_signal);
      wait_for_inferior ();
    }
  while (stop_signal != TARGET_SIGNAL_TRAP);
  stop_soon_quietly = 0;

#if !defined(_SCO_DS)
  /* We are now either at the "mapping complete" breakpoint (or somewhere
     else, a condition we aren't prepared to deal with anyway), so adjust
     the PC as necessary after a breakpoint, disable the breakpoint, and
     add any shared libraries that were mapped in. */

  if (DECR_PC_AFTER_BREAK)
    {
      stop_pc -= DECR_PC_AFTER_BREAK;
      write_register (PC_REGNUM, stop_pc);
    }

  if (!disable_break ())
    {
      warning ("shared library handler failed to disable breakpoint");
    }

  if (auto_solib_add)
    solib_add ((char *) 0, 0, (struct target_ops *) 0);
#endif /* ! _SCO_DS */
#endif
}

/*

   LOCAL FUNCTION

   special_symbol_handling -- additional shared library symbol handling

   SYNOPSIS

   void special_symbol_handling ()

   DESCRIPTION

   Once the symbols from a shared object have been loaded in the usual
   way, we are called to do any system specific symbol handling that 
   is needed.

   For SunOS4, this consists of grunging around in the dynamic
   linkers structures to find symbol definitions for "common" symbols
   and adding them to the minimal symbol table for the runtime common
   objfile.

 */

static void
special_symbol_handling ()
{
#ifndef SVR4_SHARED_LIBS
  int j;

  if (debug_addr == 0)
    {
      /* Get link_dynamic structure */

      j = target_read_memory (debug_base, (char *) &dynamic_copy,
			      sizeof (dynamic_copy));
      if (j)
	{
	  /* unreadable */
	  return;
	}

      /* Calc address of debugger interface structure */
      /* FIXME, this needs work for cross-debugging of core files
         (byteorder, size, alignment, etc).  */

      debug_addr = SOLIB_EXTRACT_ADDRESS (dynamic_copy.ldd);
    }

  /* Read the debugger structure from the inferior, just to make sure
     we have a current copy. */

  j = target_read_memory (debug_addr, (char *) &debug_copy,
			  sizeof (debug_copy));
  if (j)
    return;			/* unreadable */

  /* Get common symbol definitions for the loaded object. */

  if (debug_copy.ldd_cp)
    {
      solib_add_common_symbols (SOLIB_EXTRACT_ADDRESS (debug_copy.ldd_cp));
    }

#endif /* !SVR4_SHARED_LIBS */
}


/*

   LOCAL FUNCTION

   sharedlibrary_command -- handle command to explicitly add library

   SYNOPSIS

   static void sharedlibrary_command (char *args, int from_tty)

   DESCRIPTION

 */

static void
sharedlibrary_command (args, from_tty)
     char *args;
     int from_tty;
{
  dont_repeat ();
  solib_add (args, from_tty, (struct target_ops *) 0);
}

#endif /* HAVE_LINK_H */

void
_initialize_solib ()
{
#ifdef HAVE_LINK_H

  add_com ("sharedlibrary", class_files, sharedlibrary_command,
	   "Load shared object library symbols for files matching REGEXP.");
  add_info ("sharedlibrary", info_sharedlibrary_command,
	    "Status of loaded shared object libraries.");

  add_show_from_set
    (add_set_cmd ("auto-solib-add", class_support, var_zinteger,
		  (char *) &auto_solib_add,
		  "Set autoloading of shared library symbols.\n\
If nonzero, symbols from all shared object libraries will be loaded\n\
automatically when the inferior begins execution or when the dynamic linker\n\
informs gdb that a new library has been loaded.  Otherwise, symbols\n\
must be loaded manually, using `sharedlibrary'.",
		  &setlist),
     &showlist);

  add_show_from_set
    (add_set_cmd ("solib-absolute-prefix", class_support, var_filename,
		  (char *) &solib_absolute_prefix,
		  "Set prefix for loading absolute shared library symbol files.\n\
For other (relative) files, you can add values using `set solib-search-path'.",
		  &setlist),
     &showlist);
  add_show_from_set
    (add_set_cmd ("solib-search-path", class_support, var_string,
		  (char *) &solib_search_path,
		  "Set the search path for loading non-absolute shared library symbol files.\n\
This takes precedence over the environment variables PATH and LD_LIBRARY_PATH.",
		  &setlist),
     &showlist);

#endif /* HAVE_LINK_H */
}
