/* Shared library declarations for GDB, the GNU Debugger.

   Copyright (C) 1992-2025 Free Software Foundation, Inc.

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

#ifndef GDB_SOLIB_H
#define GDB_SOLIB_H

#include "gdb_bfd.h"
#include "gdbsupport/function-view.h"
#include "gdbsupport/intrusive_list.h"
#include "gdbsupport/owning_intrusive_list.h"
#include "symfile-add-flags.h"
#include "target-section.h"

struct program_space;

/* Value of the 'set debug solib' configuration variable.  */

extern bool debug_solib;

/* Print an "solib" debug statement.  */

#define solib_debug_printf(fmt, ...) \
  debug_prefixed_printf_cond (debug_solib, "solib", fmt, ##__VA_ARGS__)

#define SOLIB_SCOPED_DEBUG_START_END(fmt, ...) \
  scoped_debug_start_end (debug_solib, "solib", fmt, ##__VA_ARGS__)

#define SO_NAME_MAX_PATH_SIZE 512   /* FIXME: Should be dynamic */

/* Base class for target-specific link map information.  */

struct lm_info
{
  lm_info () = default;
  lm_info (const lm_info &) = default;
  virtual ~lm_info () = 0;
};

using lm_info_up = std::unique_ptr<lm_info>;

struct solib_ops;

struct solib : intrusive_list_node<solib>
{
  /* Constructor

     OPS is the solib_ops implementation providing this solib.  */
  explicit solib (const solib_ops &ops) : m_ops (&ops) {}

  /* Return the solib_ops implementation providing this solib.  */
  const solib_ops &ops () const
  { return *m_ops; }

  /* Free symbol-file related contents of SO and reset for possible reloading
     of SO.  If we have opened a BFD for SO, close it.  If we have placed SO's
     sections in some target's section table, the caller is responsible for
     removing them.

     This function doesn't mess with objfiles at all.  If there is an
     objfile associated with SO that needs to be removed, the caller is
     responsible for taking care of that.  */
  void clear () ;

  /* The following fields of the structure come directly from the
     dynamic linker's tables in the inferior, and are initialized by
     current_sos.  */

  /* A pointer to target specific link map information.  Often this
     will be a copy of struct link_map from the user process, but
     it need not be; it can be any collection of data needed to
     traverse the dynamic linker's data structures.  */
  lm_info_up lm_info;

  /* Shared object file name, exactly as it appears in the
     inferior's link map.  This may be a relative path, or something
     which needs to be looked up in LD_LIBRARY_PATH, etc.  We use it
     to tell which entries in the inferior's dynamic linker's link
     map we've already loaded.  */
  std::string original_name;

  /* Shared object file name, expanded to something GDB can open.  */
  std::string name;

  /* The following fields of the structure are built from
     information gathered from the shared object file itself, and
     are set when we actually add it to our symbol tables.

     current_sos must initialize these fields to 0.  */

  gdb_bfd_ref_ptr abfd;

  /* True if symbols have been read in.  */
  bool symbols_loaded = false;

  /* objfile with symbols for a loaded library.  Target memory is read from
     ABFD.  OBJFILE may be NULL either before symbols have been loaded, if
     the file cannot be found or after the command "nosharedlibrary".  */
  struct objfile *objfile = nullptr;

  std::vector<target_section> sections;

  /* Record the range of addresses belonging to this shared library.
     There may not be just one (e.g. if two segments are relocated
     differently).  This is used for "info sharedlibrary" and
     the MI command "-file-list-shared-libraries".  The latter has a format
     that supports outputting multiple segments once the related code
     supports them.  */
  CORE_ADDR addr_low = 0, addr_high = 0;

private:
  /* The solib_ops responsible for this solib.  */
  const solib_ops *m_ops;
};

/* A unique pointer to an solib.  */
using solib_up = std::unique_ptr<solib>;

/* Callback type for the 'iterate_over_objfiles_in_search_order'
   methods.  */

using iterate_over_objfiles_in_search_order_cb_ftype
  = gdb::function_view<bool (objfile *)>;

struct solib_ops
{
  explicit solib_ops (program_space *pspace)
    : m_pspace (pspace)
  {}

  virtual ~solib_ops () = default;

  /* Adjust the section binding addresses by the base address at
     which the object was actually mapped.  */
  virtual void relocate_section_addresses (solib &so, target_section *) const
    = 0;

  /* Reset private data structures associated with SO.
     This is called when SO is about to be reloaded.
     It is also called when SO is about to be freed.

     Defaults to no-op.  */
  virtual void clear_so (const solib &so) const {}

  /* Free private data structures associated to PSPACE.  This method
     should not free resources associated to individual solib entries,
     those are cleared by the clear_so method.

     Defaults to no-op.  */
  virtual void clear_solib (program_space *pspace) const {}

  /* Target dependent code to run after child process fork.

     Defaults to no-op.  */
  virtual void create_inferior_hook (int from_tty) const {};

  /* Construct a list of the currently loaded shared objects.  This
     list does not include an entry for the main executable file.

     Note that we only gather information directly available from the
     inferior --- we don't examine any of the shared library files
     themselves.  The declaration of `struct solib' says which fields
     we provide values for.  */
  virtual owning_intrusive_list<solib> current_sos () const = 0;

  /* Find, open, and read the symbols for the main executable.  If
     FROM_TTY is non-zero, allow messages to be printed.

     Return true if this was done successfully.  Defaults to false.  */
  virtual bool open_symbol_file_object (int from_tty) const { return false; }

  /* Determine if PC lies in the dynamic symbol resolution code of
     the run time loader.

     Defaults to false.  */
  virtual bool in_dynsym_resolve_code (CORE_ADDR pc) const
  { return false; };

  /* Find and open shared library binary file.  */
  virtual gdb_bfd_ref_ptr bfd_open (const char *pathname) const;

  /* Given two solib objects, GDB from the GDB thread list and INFERIOR from the
     list returned by current_sos, return true if they represent the same library.

     Defaults to comparing the solib original names using filename_cmp.  */
  virtual bool same (const solib &gdb, const solib &inferior) const;

  /* Return whether a region of memory must be kept in a core file
     for shared libraries loaded before "gcore" is used to be
     handled correctly when the core file is loaded.  This only
     applies when the section would otherwise not be kept in the
     core file (in particular, for readonly sections).

     Defaults to false.  */
  virtual bool keep_data_in_core (CORE_ADDR vaddr, unsigned long size) const
  { return false; };

  /* Enable or disable optional solib event breakpoints as appropriate.  This
     should be called whenever stop_on_solib_events is changed.

     Defaults to no-op.  */
  virtual void update_breakpoints () const {};

  /* Target-specific processing of solib events that will be performed before
     solib_add is called.

     Defaults to no-op.  */
  virtual void handle_event () const {};

  /* Return an address within the inferior's address space which is known
     to be part of SO.  If there is no such address, or GDB doesn't know
     how to figure out such an address then an empty optional is
     returned.

     The returned address can be used when loading the shared libraries
     for a core file.  GDB knows the build-ids for (some) files mapped
     into the inferior's address space, and knows the address ranges which
     those mapped files cover.  If GDB can figure out a representative
     address for the library then this can be used to match a library to a
     mapped file, and thus to a build-id.  GDB can then use this
     information to help locate the shared library objfile, if the objfile
     is not in the expected place (as defined by the shared libraries file
     name).

     The default implementation of returns an empty option, indicating GDB is
     unable to find an address within the library SO.  */
  virtual std::optional<CORE_ADDR> find_solib_addr (solib &so) const
  { return {}; };

  /* Return true if the linker or libc supports linkage namespaces.

     Defaults to false.  */
  virtual bool supports_namespaces () const { return false; }

  /* Return which linker namespace contains SO.

     The supports_namespaces method must return true for this to be
     called.

     Throw an error if the namespace can not be determined (such as when we're
     stepping though the dynamic linker).  */
  virtual int find_solib_ns (const solib &so) const
  { gdb_assert_not_reached ("namespaces not supported"); }

  /* Returns the number of active namespaces in the inferior.

     The supports_namespaces method must return true for this to be called.  */
  virtual int num_active_namespaces () const
  { gdb_assert_not_reached ("namespaces not supported"); }

  /* Returns all solibs for a given namespace.  If the namespace is not
     active, returns an empty vector.

     The supports_namespaces method must return true for this to be called.  */
  virtual std::vector<const solib *> get_solibs_in_ns (int ns) const
  { gdb_assert_not_reached ("namespaces not supported"); }

  /* Iterate over all objfiles of the program space in the order that makes the
     most sense for the architecture to make global symbol searches.

     CB is a callback function passed an objfile to be searched.  The iteration
     stops if this function returns true.

     If not nullptr, CURRENT_OBJFILE corresponds to the objfile being inspected
     when the symbol search was requested.  */
  virtual void iterate_over_objfiles_in_search_order
    (iterate_over_objfiles_in_search_order_cb_ftype cb,
     objfile *current_objfile) const;

protected:
  /* The program space for which this solib_ops was created.  */
  program_space *m_pspace;
};

/* A unique pointer to an solib_ops.  */
using solib_ops_up = std::unique_ptr<solib_ops>;

/* Find main executable binary file.  */
extern gdb::unique_xmalloc_ptr<char> exec_file_find (const char *in_pathname,
						     int *fd);

/* Find shared library binary file.  */
extern gdb::unique_xmalloc_ptr<char> solib_find (const char *in_pathname,
						 int *fd);

/* Open BFD for shared library file.  */
extern gdb_bfd_ref_ptr solib_bfd_fopen (const char *pathname, int fd);

/* Find solib binary file and open it.  */
extern gdb_bfd_ref_ptr solib_bfd_open (const char *in_pathname);

/* Called when we free all symtabs of PSPACE, to free the shared library
   information as well.  */

extern void clear_solib (program_space *pspace);

/* Called to add symbols from a shared library to gdb's symbol table.  */

extern void solib_add (const char *, int, int);
extern bool solib_read_symbols (solib &, symfile_add_flags);

/* Function to be called when the inferior starts up, to discover the
   names of shared libraries that are dynamically linked, the base
   addresses to which they are linked, and sufficient information to
   read in their symbols at a later time.  */

extern void solib_create_inferior_hook (int from_tty);

/* If ADDR lies in a shared library, return its name.  */

extern const char *solib_name_from_address (struct program_space *, CORE_ADDR);

/* Return true if ADDR lies within SOLIB.  */

extern bool solib_contains_address_p (const solib &, CORE_ADDR);

/* Return whether the data starting at VADDR, size SIZE, must be kept
   in a core file for shared libraries loaded before "gcore" is used
   to be handled correctly when the core file is loaded.  This only
   applies when the section would otherwise not be kept in the core
   file (in particular, for readonly sections).  */

extern bool solib_keep_data_in_core (CORE_ADDR vaddr, unsigned long size);

/* Return true if PC lies in the dynamic symbol resolution code of the
   run time loader.  */

extern bool in_solib_dynsym_resolve_code (CORE_ADDR);

/* Discard symbols that were auto-loaded from shared libraries in PSPACE.  */

extern void no_shared_libraries (program_space *pspace);

/* Synchronize GDB's shared object list with inferior's.

   Extract the list of currently loaded shared objects from the
   inferior, and compare it with the list of shared objects in the
   current program space's list of shared libraries.  Edit
   the current program space's solib list to bring it in sync with the
   inferior's new list.

   If we notice that the inferior has unloaded some shared objects,
   free any symbolic info GDB had read about those shared objects.

   Don't load symbolic info for any new shared objects; just add them
   to the list, and leave their symbols_loaded flag clear.

   If FROM_TTY is non-null, feel free to print messages about what
   we're doing.  */

extern void update_solib_list (int from_tty);

/* Return true if NAME is the libpthread shared library.  */

extern bool libpthread_name_p (const char *name);

/* Look up symbol from both symbol table and dynamic string table.  */

extern CORE_ADDR gdb_bfd_lookup_symbol
     (bfd *abfd, gdb::function_view<bool (const asymbol *)> match_sym);

/* Look up symbol from symbol table.  */

extern CORE_ADDR gdb_bfd_lookup_symbol_from_symtab
     (bfd *abfd, gdb::function_view<bool (const asymbol *)> match_sym);

/* Scan for DESIRED_DYNTAG in .dynamic section of ABFD.  If DESIRED_DYNTAG is
   found, 1 is returned and the corresponding PTR and PTR_ADDR are set.  */

extern int gdb_bfd_scan_elf_dyntag (const int desired_dyntag, bfd *abfd,
				    CORE_ADDR *ptr, CORE_ADDR *ptr_addr);

/* If FILENAME refers to an ELF shared object then attempt to return the
   string referred to by its DT_SONAME tag.   */

extern gdb::unique_xmalloc_ptr<char> gdb_bfd_read_elf_soname
  (const char *filename);

/* Enable or disable optional solib event breakpoints as appropriate.  */

extern void update_solib_breakpoints (void);

/* Handle an solib event by calling solib_add.  */

extern void handle_solib_event (void);

/* Calculate the number of linker namespaces active in PSPACE.  */

extern int solib_linker_namespace_count (program_space *pspace);

#endif /* GDB_SOLIB_H */
