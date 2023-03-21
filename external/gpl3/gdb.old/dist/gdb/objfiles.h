/* Definitions for symbol file management in GDB.

   Copyright (C) 1992-2020 Free Software Foundation, Inc.

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

#if !defined (OBJFILES_H)
#define OBJFILES_H

#include "hashtab.h"
#include "gdb_obstack.h"	/* For obstack internals.  */
#include "objfile-flags.h"
#include "symfile.h"
#include "progspace.h"
#include "registry.h"
#include "gdb_bfd.h"
#include "psymtab.h"
#include <atomic>
#include <bitset>
#include <vector>
#include "gdbsupport/next-iterator.h"
#include "gdbsupport/safe-iterator.h"
#include "bcache.h"
#include "gdbarch.h"
#include "gdbsupport/refcounted-object.h"
#include "jit.h"

struct htab;
struct objfile_data;
struct partial_symbol;

/* This structure maintains information on a per-objfile basis about the
   "entry point" of the objfile, and the scope within which the entry point
   exists.  It is possible that gdb will see more than one objfile that is
   executable, each with its own entry point.

   For example, for dynamically linked executables in SVR4, the dynamic linker
   code is contained within the shared C library, which is actually executable
   and is run by the kernel first when an exec is done of a user executable
   that is dynamically linked.  The dynamic linker within the shared C library
   then maps in the various program segments in the user executable and jumps
   to the user executable's recorded entry point, as if the call had been made
   directly by the kernel.

   The traditional gdb method of using this info was to use the
   recorded entry point to set the entry-file's lowpc and highpc from
   the debugging information, where these values are the starting
   address (inclusive) and ending address (exclusive) of the
   instruction space in the executable which correspond to the
   "startup file", i.e. crt0.o in most cases.  This file is assumed to
   be a startup file and frames with pc's inside it are treated as
   nonexistent.  Setting these variables is necessary so that
   backtraces do not fly off the bottom of the stack.

   NOTE: cagney/2003-09-09: It turns out that this "traditional"
   method doesn't work.  Corinna writes: ``It turns out that the call
   to test for "inside entry file" destroys a meaningful backtrace
   under some conditions.  E.g. the backtrace tests in the asm-source
   testcase are broken for some targets.  In this test the functions
   are all implemented as part of one file and the testcase is not
   necessarily linked with a start file (depending on the target).
   What happens is, that the first frame is printed normally and
   following frames are treated as being inside the entry file then.
   This way, only the #0 frame is printed in the backtrace output.''
   Ref "frame.c" "NOTE: vinschen/2003-04-01".

   Gdb also supports an alternate method to avoid running off the bottom
   of the stack.

   There are two frames that are "special", the frame for the function
   containing the process entry point, since it has no predecessor frame,
   and the frame for the function containing the user code entry point
   (the main() function), since all the predecessor frames are for the
   process startup code.  Since we have no guarantee that the linked
   in startup modules have any debugging information that gdb can use,
   we need to avoid following frame pointers back into frames that might
   have been built in the startup code, as we might get hopelessly
   confused.  However, we almost always have debugging information
   available for main().

   These variables are used to save the range of PC values which are
   valid within the main() function and within the function containing
   the process entry point.  If we always consider the frame for
   main() as the outermost frame when debugging user code, and the
   frame for the process entry point function as the outermost frame
   when debugging startup code, then all we have to do is have
   DEPRECATED_FRAME_CHAIN_VALID return false whenever a frame's
   current PC is within the range specified by these variables.  In
   essence, we set "ceilings" in the frame chain beyond which we will
   not proceed when following the frame chain back up the stack.

   A nice side effect is that we can still debug startup code without
   running off the end of the frame chain, assuming that we have usable
   debugging information in the startup modules, and if we choose to not
   use the block at main, or can't find it for some reason, everything
   still works as before.  And if we have no startup code debugging
   information but we do have usable information for main(), backtraces
   from user code don't go wandering off into the startup code.  */

struct entry_info
{
  /* The unrelocated value we should use for this objfile entry point.  */
  CORE_ADDR entry_point;

  /* The index of the section in which the entry point appears.  */
  int the_bfd_section_index;

  /* Set to 1 iff ENTRY_POINT contains a valid value.  */
  unsigned entry_point_p : 1;

  /* Set to 1 iff this object was initialized.  */
  unsigned initialized : 1;
};

/* Sections in an objfile.  The section offsets are stored in the
   OBJFILE.  */

struct obj_section
{
  /* BFD section pointer */
  struct bfd_section *the_bfd_section;

  /* Objfile this section is part of.  */
  struct objfile *objfile;

  /* True if this "overlay section" is mapped into an "overlay region".  */
  int ovly_mapped;
};

/* Relocation offset applied to S.  */
#define obj_section_offset(s)						\
  (((s)->objfile->section_offsets)[gdb_bfd_section_index ((s)->objfile->obfd, (s)->the_bfd_section)])

/* The memory address of section S (vma + offset).  */
#define obj_section_addr(s)				      		\
  (bfd_section_vma (s->the_bfd_section)					\
   + obj_section_offset (s))

/* The one-passed-the-end memory address of section S
   (vma + size + offset).  */
#define obj_section_endaddr(s)						\
  (bfd_section_vma (s->the_bfd_section)					\
   + bfd_section_size ((s)->the_bfd_section)				\
   + obj_section_offset (s))

#define ALL_OBJFILE_OSECTIONS(objfile, osect)	\
  for (osect = objfile->sections; osect < objfile->sections_end; osect++) \
    if (osect->the_bfd_section == NULL)					\
      {									\
	/* Nothing.  */							\
      }									\
    else

#define SECT_OFF_DATA(objfile) \
     ((objfile->sect_index_data == -1) \
      ? (internal_error (__FILE__, __LINE__, \
			 _("sect_index_data not initialized")), -1)	\
      : objfile->sect_index_data)

#define SECT_OFF_RODATA(objfile) \
     ((objfile->sect_index_rodata == -1) \
      ? (internal_error (__FILE__, __LINE__, \
			 _("sect_index_rodata not initialized")), -1)	\
      : objfile->sect_index_rodata)

#define SECT_OFF_TEXT(objfile) \
     ((objfile->sect_index_text == -1) \
      ? (internal_error (__FILE__, __LINE__, \
			 _("sect_index_text not initialized")), -1)	\
      : objfile->sect_index_text)

/* Sometimes the .bss section is missing from the objfile, so we don't
   want to die here.  Let the users of SECT_OFF_BSS deal with an
   uninitialized section index.  */
#define SECT_OFF_BSS(objfile) (objfile)->sect_index_bss

/* The "objstats" structure provides a place for gdb to record some
   interesting information about its internal state at runtime, on a
   per objfile basis, such as information about the number of symbols
   read, size of string table (if any), etc.  */

struct objstats
{
  /* Number of partial symbols read.  */
  int n_psyms = 0;

  /* Number of full symbols read.  */
  int n_syms = 0;

  /* Number of ".stabs" read (if applicable).  */
  int n_stabs = 0;

  /* Number of types.  */
  int n_types = 0;

  /* Size of stringtable, (if applicable).  */
  int sz_strtab = 0;
};

#define OBJSTAT(objfile, expr) (objfile -> stats.expr)
#define OBJSTATS struct objstats stats
extern void print_objfile_statistics (void);
extern void print_symbol_bcache_statistics (void);

/* Number of entries in the minimal symbol hash table.  */
#define MINIMAL_SYMBOL_HASH_SIZE 2039

/* An iterator for minimal symbols.  */

struct minimal_symbol_iterator
{
  typedef minimal_symbol_iterator self_type;
  typedef struct minimal_symbol *value_type;
  typedef struct minimal_symbol *&reference;
  typedef struct minimal_symbol **pointer;
  typedef std::forward_iterator_tag iterator_category;
  typedef int difference_type;

  explicit minimal_symbol_iterator (struct minimal_symbol *msym)
    : m_msym (msym)
  {
  }

  value_type operator* () const
  {
    return m_msym;
  }

  bool operator== (const self_type &other) const
  {
    return m_msym == other.m_msym;
  }

  bool operator!= (const self_type &other) const
  {
    return m_msym != other.m_msym;
  }

  self_type &operator++ ()
  {
    ++m_msym;
    return *this;
  }

private:
  struct minimal_symbol *m_msym;
};

/* Some objfile data is hung off the BFD.  This enables sharing of the
   data across all objfiles using the BFD.  The data is stored in an
   instance of this structure, and associated with the BFD using the
   registry system.  */

struct objfile_per_bfd_storage
{
  objfile_per_bfd_storage ()
    : minsyms_read (false)
  {}

  ~objfile_per_bfd_storage ();

  /* The storage has an obstack of its own.  */

  auto_obstack storage_obstack;

  /* String cache.  */

  gdb::bcache string_cache;

  /* The gdbarch associated with the BFD.  Note that this gdbarch is
     determined solely from BFD information, without looking at target
     information.  The gdbarch determined from a running target may
     differ from this e.g. with respect to register types and names.  */

  struct gdbarch *gdbarch = NULL;

  /* Hash table for mapping symbol names to demangled names.  Each
     entry in the hash table is a demangled_name_entry struct, storing the
     language and two consecutive strings, both null-terminated; the first one
     is a mangled or linkage name, and the second is the demangled name or just
     a zero byte if the name doesn't demangle.  */

  htab_up demangled_names_hash;

  /* The per-objfile information about the entry point, the scope (file/func)
     containing the entry point, and the scope of the user's main() func.  */

  entry_info ei {};

  /* The name and language of any "main" found in this objfile.  The
     name can be NULL, which means that the information was not
     recorded.  */

  const char *name_of_main = NULL;
  enum language language_of_main = language_unknown;

  /* Each file contains a pointer to an array of minimal symbols for all
     global symbols that are defined within the file.  The array is
     terminated by a "null symbol", one that has a NULL pointer for the
     name and a zero value for the address.  This makes it easy to walk
     through the array when passed a pointer to somewhere in the middle
     of it.  There is also a count of the number of symbols, which does
     not include the terminating null symbol.  */

  gdb::unique_xmalloc_ptr<minimal_symbol> msymbols;
  int minimal_symbol_count = 0;

  /* The number of minimal symbols read, before any minimal symbol
     de-duplication is applied.  Note in particular that this has only
     a passing relationship with the actual size of the table above;
     use minimal_symbol_count if you need the true size.  */

  int n_minsyms = 0;

  /* This is true if minimal symbols have already been read.  Symbol
     readers can use this to bypass minimal symbol reading.  Also, the
     minimal symbol table management code in minsyms.c uses this to
     suppress new minimal symbols.  You might think that MSYMBOLS or
     MINIMAL_SYMBOL_COUNT could be used for this, but it is possible
     for multiple readers to install minimal symbols into a given
     per-BFD.  */

  bool minsyms_read : 1;

  /* This is a hash table used to index the minimal symbols by (mangled)
     name.  */

  minimal_symbol *msymbol_hash[MINIMAL_SYMBOL_HASH_SIZE] {};

  /* This hash table is used to index the minimal symbols by their
     demangled names.  Uses a language-specific hash function via
     search_name_hash.  */

  minimal_symbol *msymbol_demangled_hash[MINIMAL_SYMBOL_HASH_SIZE] {};

  /* All the different languages of symbols found in the demangled
     hash table.  */
  std::bitset<nr_languages> demangled_hash_languages;
};

/* An iterator that first returns a parent objfile, and then each
   separate debug objfile.  */

class separate_debug_iterator
{
public:

  explicit separate_debug_iterator (struct objfile *objfile)
    : m_objfile (objfile),
      m_parent (objfile)
  {
  }

  bool operator!= (const separate_debug_iterator &other)
  {
    return m_objfile != other.m_objfile;
  }

  separate_debug_iterator &operator++ ();

  struct objfile *operator* ()
  {
    return m_objfile;
  }

private:

  struct objfile *m_objfile;
  struct objfile *m_parent;
};

/* A range adapter wrapping separate_debug_iterator.  */

class separate_debug_range
{
public:

  explicit separate_debug_range (struct objfile *objfile)
    : m_objfile (objfile)
  {
  }

  separate_debug_iterator begin ()
  {
    return separate_debug_iterator (m_objfile);
  }

  separate_debug_iterator end ()
  {
    return separate_debug_iterator (nullptr);
  }

private:

  struct objfile *m_objfile;
};

/* Master structure for keeping track of each file from which
   gdb reads symbols.  There are several ways these get allocated: 1.
   The main symbol file, symfile_objfile, set by the symbol-file command,
   2.  Additional symbol files added by the add-symbol-file command,
   3.  Shared library objfiles, added by ADD_SOLIB,  4.  symbol files
   for modules that were loaded when GDB attached to a remote system
   (see remote-vx.c).

   GDB typically reads symbols twice -- first an initial scan which just
   reads "partial symbols"; these are partial information for the
   static/global symbols in a symbol file.  When later looking up symbols,
   objfile->sf->qf->lookup_symbol is used to check if we only have a partial
   symbol and if so, read and expand the full compunit.  */

struct objfile
{
private:

  /* The only way to create an objfile is to call objfile::make.  */
  objfile (bfd *, const char *, objfile_flags);

public:

  /* Normally you should not call delete.  Instead, call 'unlink' to
     remove it from the program space's list.  In some cases, you may
     need to hold a reference to an objfile that is independent of its
     existence on the program space's list; for this case, the
     destructor must be public so that shared_ptr can reference
     it.  */
  ~objfile ();

  /* Create an objfile.  */
  static objfile *make (bfd *bfd_, const char *name_, objfile_flags flags_,
			objfile *parent = nullptr);

  /* Remove an objfile from the current program space, and free
     it.  */
  void unlink ();

  DISABLE_COPY_AND_ASSIGN (objfile);

  /* A range adapter that makes it possible to iterate over all
     psymtabs in one objfile.  */

  psymtab_storage::partial_symtab_range psymtabs ()
  {
    return partial_symtabs->range ();
  }

  /* Reset the storage for the partial symbol tables.  */

  void reset_psymtabs ()
  {
    psymbol_map.clear ();
    partial_symtabs.reset (new psymtab_storage ());
  }

  typedef next_adapter<struct compunit_symtab> compunits_range;

  /* A range adapter that makes it possible to iterate over all
     compunits in one objfile.  */

  compunits_range compunits ()
  {
    return compunits_range (compunit_symtabs);
  }

  /* A range adapter that makes it possible to iterate over all
     minimal symbols of an objfile.  */

  class msymbols_range
  {
  public:

    explicit msymbols_range (struct objfile *objfile)
      : m_objfile (objfile)
    {
    }

    minimal_symbol_iterator begin () const
    {
      return minimal_symbol_iterator (m_objfile->per_bfd->msymbols.get ());
    }

    minimal_symbol_iterator end () const
    {
      return minimal_symbol_iterator
	(m_objfile->per_bfd->msymbols.get ()
	 + m_objfile->per_bfd->minimal_symbol_count);
    }

  private:

    struct objfile *m_objfile;
  };

  /* Return a range adapter for iterating over all minimal
     symbols.  */

  msymbols_range msymbols ()
  {
    return msymbols_range (this);
  }

  /* Return a range adapter for iterating over all the separate debug
     objfiles of this objfile.  */

  separate_debug_range separate_debug_objfiles ()
  {
    return separate_debug_range (this);
  }

  CORE_ADDR text_section_offset () const
  {
    return section_offsets[SECT_OFF_TEXT (this)];
  }

  CORE_ADDR data_section_offset () const
  {
    return section_offsets[SECT_OFF_DATA (this)];
  }

  /* Intern STRING and return the unique copy.  The copy has the same
     lifetime as the per-BFD object.  */
  const char *intern (const char *str)
  {
    return (const char *) per_bfd->string_cache.insert (str, strlen (str) + 1);
  }

  /* Intern STRING and return the unique copy.  The copy has the same
     lifetime as the per-BFD object.  */
  const char *intern (const std::string &str)
  {
    return (const char *) per_bfd->string_cache.insert (str.c_str (),
							str.size () + 1);
  }

  /* Retrieve the gdbarch associated with this objfile.  */
  struct gdbarch *arch () const
  {
    return per_bfd->gdbarch;
  }


  /* The object file's original name as specified by the user,
     made absolute, and tilde-expanded.  However, it is not canonicalized
     (i.e., it has not been passed through gdb_realpath).
     This pointer is never NULL.  This does not have to be freed; it is
     guaranteed to have a lifetime at least as long as the objfile.  */

  const char *original_name = nullptr;

  CORE_ADDR addr_low = 0;

  /* Some flag bits for this objfile.  */

  objfile_flags flags;

  /* The program space associated with this objfile.  */

  struct program_space *pspace;

  /* List of compunits.
     These are used to do symbol lookups and file/line-number lookups.  */

  struct compunit_symtab *compunit_symtabs = nullptr;

  /* The partial symbol tables.  */

  std::shared_ptr<psymtab_storage> partial_symtabs;

  /* The object file's BFD.  Can be null if the objfile contains only
     minimal symbols, e.g. the run time common symbols for SunOS4.  */

  bfd *obfd;

  /* The per-BFD data.  Note that this is treated specially if OBFD
     is NULL.  */

  struct objfile_per_bfd_storage *per_bfd = nullptr;

  /* The modification timestamp of the object file, as of the last time
     we read its symbols.  */

  long mtime = 0;

  /* Obstack to hold objects that should be freed when we load a new symbol
     table from this object file.  */

  struct obstack objfile_obstack {};

  /* Map symbol addresses to the partial symtab that defines the
     object at that address.  */

  std::vector<std::pair<CORE_ADDR, partial_symtab *>> psymbol_map;

  /* Structure which keeps track of functions that manipulate objfile's
     of the same type as this objfile.  I.e. the function to read partial
     symbols for example.  Note that this structure is in statically
     allocated memory, and is shared by all objfiles that use the
     object module reader of this type.  */

  const struct sym_fns *sf = nullptr;

  /* Per objfile data-pointers required by other GDB modules.  */

  REGISTRY_FIELDS {};

  /* Set of relocation offsets to apply to each section.
     The table is indexed by the_bfd_section->index, thus it is generally
     as large as the number of sections in the binary.

     These offsets indicate that all symbols (including partial and
     minimal symbols) which have been read have been relocated by this
     much.  Symbols which are yet to be read need to be relocated by it.  */

  ::section_offsets section_offsets;

  /* Indexes in the section_offsets array.  These are initialized by the
     *_symfile_offsets() family of functions (som_symfile_offsets,
     xcoff_symfile_offsets, default_symfile_offsets).  In theory they
     should correspond to the section indexes used by bfd for the
     current objfile.  The exception to this for the time being is the
     SOM version.

     These are initialized to -1 so that we can later detect if they
     are used w/o being properly assigned to.  */

  int sect_index_text = -1;
  int sect_index_data = -1;
  int sect_index_bss = -1;
  int sect_index_rodata = -1;

  /* These pointers are used to locate the section table, which
     among other things, is used to map pc addresses into sections.
     SECTIONS points to the first entry in the table, and
     SECTIONS_END points to the first location past the last entry
     in the table.  The table is stored on the objfile_obstack.  The
     sections are indexed by the BFD section index; but the
     structure data is only valid for certain sections
     (e.g. non-empty, SEC_ALLOC).  */

  struct obj_section *sections = nullptr;
  struct obj_section *sections_end = nullptr;

  /* GDB allows to have debug symbols in separate object files.  This is
     used by .gnu_debuglink, ELF build id note and Mach-O OSO.
     Although this is a tree structure, GDB only support one level
     (ie a separate debug for a separate debug is not supported).  Note that
     separate debug object are in the main chain and therefore will be
     visited by objfiles & co iterators.  Separate debug objfile always
     has a non-nul separate_debug_objfile_backlink.  */

  /* Link to the first separate debug object, if any.  */

  struct objfile *separate_debug_objfile = nullptr;

  /* If this is a separate debug object, this is used as a link to the
     actual executable objfile.  */

  struct objfile *separate_debug_objfile_backlink = nullptr;

  /* If this is a separate debug object, this is a link to the next one
     for the same executable objfile.  */

  struct objfile *separate_debug_objfile_link = nullptr;

  /* Place to stash various statistics about this objfile.  */

  OBJSTATS;

  /* A linked list of symbols created when reading template types or
     function templates.  These symbols are not stored in any symbol
     table, so we have to keep them here to relocate them
     properly.  */

  struct symbol *template_symbols = nullptr;

  /* Associate a static link (struct dynamic_prop *) to all blocks (struct
     block *) that have one.

     In the context of nested functions (available in Pascal, Ada and GNU C,
     for instance), a static link (as in DWARF's DW_AT_static_link attribute)
     for a function is a way to get the frame corresponding to the enclosing
     function.

     Very few blocks have a static link, so it's more memory efficient to
     store these here rather than in struct block.  Static links must be
     allocated on the objfile's obstack.  */
  htab_up static_links;

  /* JIT-related data for this objfile, if the objfile is a JITer;
     that is, it produces JITed objfiles.  */
  std::unique_ptr<jiter_objfile_data> jiter_data = nullptr;

  /* JIT-related data for this objfile, if the objfile is JITed;
     that is, it was produced by a JITer.  */
  std::unique_ptr<jited_objfile_data> jited_data = nullptr;

  /* A flag that is set to true if the JIT interface symbols are not
     found in this objfile, so that we can skip the symbol lookup the
     next time.  If an objfile does not have the symbols, it will
     never have them.  */
  bool skip_jit_symbol_lookup = false;
};

/* A deleter for objfile.  */

struct objfile_deleter
{
  void operator() (objfile *ptr) const
  {
    ptr->unlink ();
  }
};

/* A unique pointer that holds an objfile.  */

typedef std::unique_ptr<objfile, objfile_deleter> objfile_up;

/* Declarations for functions defined in objfiles.c */

extern int entry_point_address_query (CORE_ADDR *entry_p);

extern CORE_ADDR entry_point_address (void);

extern void build_objfile_section_table (struct objfile *);

extern void free_objfile_separate_debug (struct objfile *);

extern void objfile_relocate (struct objfile *, const section_offsets &);
extern void objfile_rebase (struct objfile *, CORE_ADDR);

extern int objfile_has_partial_symbols (struct objfile *objfile);

extern int objfile_has_full_symbols (struct objfile *objfile);

extern int objfile_has_symbols (struct objfile *objfile);

extern int have_partial_symbols (void);

extern int have_full_symbols (void);

extern void objfile_set_sym_fns (struct objfile *objfile,
				 const struct sym_fns *sf);

extern void objfiles_changed (void);

/* Return true if ADDR maps into one of the sections of OBJFILE and false
   otherwise.  */

extern bool is_addr_in_objfile (CORE_ADDR addr, const struct objfile *objfile);

/* Return true if ADDRESS maps into one of the sections of a
   OBJF_SHARED objfile of PSPACE and false otherwise.  */

extern bool shared_objfile_contains_address_p (struct program_space *pspace,
                                               CORE_ADDR address);

/* This operation deletes all objfile entries that represent solibs that
   weren't explicitly loaded by the user, via e.g., the add-symbol-file
   command.  */

extern void objfile_purge_solibs (void);

/* Functions for dealing with the minimal symbol table, really a misc
   address<->symbol mapping for things we don't have debug symbols for.  */

extern int have_minimal_symbols (void);

extern struct obj_section *find_pc_section (CORE_ADDR pc);

/* Return non-zero if PC is in a section called NAME.  */
extern int pc_in_section (CORE_ADDR, const char *);

/* Return non-zero if PC is in a SVR4-style procedure linkage table
   section.  */

static inline int
in_plt_section (CORE_ADDR pc)
{
  return pc_in_section (pc, ".plt");
}

/* Keep a registry of per-objfile data-pointers required by other GDB
   modules.  */
DECLARE_REGISTRY(objfile);

/* In normal use, the section map will be rebuilt by find_pc_section
   if objfiles have been added, removed or relocated since it was last
   called.  Calling inhibit_section_map_updates will inhibit this
   behavior until the returned scoped_restore object is destroyed.  If
   you call inhibit_section_map_updates you must ensure that every
   call to find_pc_section in the inhibited region relates to a
   section that is already in the section map and has not since been
   removed or relocated.  */
extern scoped_restore_tmpl<int> inhibit_section_map_updates
    (struct program_space *pspace);

extern void default_iterate_over_objfiles_in_search_order
  (struct gdbarch *gdbarch,
   iterate_over_objfiles_in_search_order_cb_ftype *cb,
   void *cb_data, struct objfile *current_objfile);

/* Reset the per-BFD storage area on OBJ.  */

void set_objfile_per_bfd (struct objfile *obj);

/* Return canonical name for OBJFILE.
   This is the real file name if the file has been opened.
   Otherwise it is the original name supplied by the user.  */

const char *objfile_name (const struct objfile *objfile);

/* Return the (real) file name of OBJFILE if the file has been opened,
   otherwise return NULL.  */

const char *objfile_filename (const struct objfile *objfile);

/* Return the name to print for OBJFILE in debugging messages.  */

extern const char *objfile_debug_name (const struct objfile *objfile);

/* Return the name of the file format of OBJFILE if the file has been opened,
   otherwise return NULL.  */

const char *objfile_flavour_name (struct objfile *objfile);

/* Set the objfile's notion of the "main" name and language.  */

extern void set_objfile_main_name (struct objfile *objfile,
				   const char *name, enum language lang);

extern void objfile_register_static_link
  (struct objfile *objfile,
   const struct block *block,
   const struct dynamic_prop *static_link);

extern const struct dynamic_prop *objfile_lookup_static_link
  (struct objfile *objfile, const struct block *block);

#endif /* !defined (OBJFILES_H) */
