/* Handle JIT code generation in the inferior for GDB, the GNU Debugger.

   Copyright (C) 2009-2020 Free Software Foundation, Inc.

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

#include "jit.h"
#include "jit-reader.h"
#include "block.h"
#include "breakpoint.h"
#include "command.h"
#include "dictionary.h"
#include "filenames.h"
#include "frame-unwind.h"
#include "gdbcmd.h"
#include "gdbcore.h"
#include "inferior.h"
#include "observable.h"
#include "objfiles.h"
#include "regcache.h"
#include "symfile.h"
#include "symtab.h"
#include "target.h"
#include "gdbsupport/gdb-dlfcn.h"
#include <sys/stat.h>
#include "gdb_bfd.h"
#include "readline/tilde.h"
#include "completer.h"
#include <forward_list>

static std::string jit_reader_dir;

static const char *const jit_break_name = "__jit_debug_register_code";

static const char *const jit_descriptor_name = "__jit_debug_descriptor";

static void jit_inferior_init (struct gdbarch *gdbarch);
static void jit_inferior_exit_hook (struct inferior *inf);

/* An unwinder is registered for every gdbarch.  This key is used to
   remember if the unwinder has been registered for a particular
   gdbarch.  */

static struct gdbarch_data *jit_gdbarch_data;

/* Non-zero if we want to see trace of jit level stuff.  */

static unsigned int jit_debug = 0;

static void
show_jit_debug (struct ui_file *file, int from_tty,
		struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("JIT debugging is %s.\n"), value);
}

struct target_buffer
{
  CORE_ADDR base;
  ULONGEST size;
};

/* Opening the file is a no-op.  */

static void *
mem_bfd_iovec_open (struct bfd *abfd, void *open_closure)
{
  return open_closure;
}

/* Closing the file is just freeing the base/size pair on our side.  */

static int
mem_bfd_iovec_close (struct bfd *abfd, void *stream)
{
  xfree (stream);

  /* Zero means success.  */
  return 0;
}

/* For reading the file, we just need to pass through to target_read_memory and
   fix up the arguments and return values.  */

static file_ptr
mem_bfd_iovec_pread (struct bfd *abfd, void *stream, void *buf,
		     file_ptr nbytes, file_ptr offset)
{
  int err;
  struct target_buffer *buffer = (struct target_buffer *) stream;

  /* If this read will read all of the file, limit it to just the rest.  */
  if (offset + nbytes > buffer->size)
    nbytes = buffer->size - offset;

  /* If there are no more bytes left, we've reached EOF.  */
  if (nbytes == 0)
    return 0;

  err = target_read_memory (buffer->base + offset, (gdb_byte *) buf, nbytes);
  if (err)
    return -1;

  return nbytes;
}

/* For statting the file, we only support the st_size attribute.  */

static int
mem_bfd_iovec_stat (struct bfd *abfd, void *stream, struct stat *sb)
{
  struct target_buffer *buffer = (struct target_buffer*) stream;

  memset (sb, 0, sizeof (struct stat));
  sb->st_size = buffer->size;
  return 0;
}

/* Open a BFD from the target's memory.  */

static gdb_bfd_ref_ptr
bfd_open_from_target_memory (CORE_ADDR addr, ULONGEST size,
			     const char *target)
{
  struct target_buffer *buffer = XNEW (struct target_buffer);

  buffer->base = addr;
  buffer->size = size;
  return gdb_bfd_openr_iovec ("<in-memory>", target,
			      mem_bfd_iovec_open,
			      buffer,
			      mem_bfd_iovec_pread,
			      mem_bfd_iovec_close,
			      mem_bfd_iovec_stat);
}

struct jit_reader
{
  jit_reader (struct gdb_reader_funcs *f, gdb_dlhandle_up &&h)
    : functions (f), handle (std::move (h))
  {
  }

  ~jit_reader ()
  {
    functions->destroy (functions);
  }

  DISABLE_COPY_AND_ASSIGN (jit_reader);

  struct gdb_reader_funcs *functions;
  gdb_dlhandle_up handle;
};

/* One reader that has been loaded successfully, and can potentially be used to
   parse debug info.  */

static struct jit_reader *loaded_jit_reader = NULL;

typedef struct gdb_reader_funcs * (reader_init_fn_type) (void);
static const char *reader_init_fn_sym = "gdb_init_reader";

/* Try to load FILE_NAME as a JIT debug info reader.  */

static struct jit_reader *
jit_reader_load (const char *file_name)
{
  reader_init_fn_type *init_fn;
  struct gdb_reader_funcs *funcs = NULL;

  if (jit_debug)
    fprintf_unfiltered (gdb_stdlog, _("Opening shared object %s.\n"),
			file_name);
  gdb_dlhandle_up so = gdb_dlopen (file_name);

  init_fn = (reader_init_fn_type *) gdb_dlsym (so, reader_init_fn_sym);
  if (!init_fn)
    error (_("Could not locate initialization function: %s."),
	   reader_init_fn_sym);

  if (gdb_dlsym (so, "plugin_is_GPL_compatible") == NULL)
    error (_("Reader not GPL compatible."));

  funcs = init_fn ();
  if (funcs->reader_version != GDB_READER_INTERFACE_VERSION)
    error (_("Reader version does not match GDB version."));

  return new jit_reader (funcs, std::move (so));
}

/* Provides the jit-reader-load command.  */

static void
jit_reader_load_command (const char *args, int from_tty)
{
  if (args == NULL)
    error (_("No reader name provided."));
  gdb::unique_xmalloc_ptr<char> file (tilde_expand (args));

  if (loaded_jit_reader != NULL)
    error (_("JIT reader already loaded.  Run jit-reader-unload first."));

  if (!IS_ABSOLUTE_PATH (file.get ()))
    file.reset (xstrprintf ("%s%s%s", jit_reader_dir.c_str (), SLASH_STRING,
			    file.get ()));

  loaded_jit_reader = jit_reader_load (file.get ());
  reinit_frame_cache ();
  jit_inferior_created_hook ();
}

/* Provides the jit-reader-unload command.  */

static void
jit_reader_unload_command (const char *args, int from_tty)
{
  if (!loaded_jit_reader)
    error (_("No JIT reader loaded."));

  reinit_frame_cache ();
  jit_inferior_exit_hook (current_inferior ());

  delete loaded_jit_reader;
  loaded_jit_reader = NULL;
}

/* Destructor for jiter_objfile_data.  */

jiter_objfile_data::~jiter_objfile_data ()
{
  if (this->jit_breakpoint != nullptr)
    delete_breakpoint (this->jit_breakpoint);
}

/* Fetch the jiter_objfile_data associated with OBJF.  If no data exists
   yet, make a new structure and attach it.  */

static jiter_objfile_data *
get_jiter_objfile_data (objfile *objf)
{
  if (objf->jiter_data == nullptr)
    objf->jiter_data.reset (new jiter_objfile_data ());

  return objf->jiter_data.get ();
}

/* Remember OBJFILE has been created for struct jit_code_entry located
   at inferior address ENTRY.  */

static void
add_objfile_entry (struct objfile *objfile, CORE_ADDR entry)
{
  gdb_assert (objfile->jited_data == nullptr);

  objfile->jited_data.reset (new jited_objfile_data (entry));
}

/* Helper function for reading the global JIT descriptor from remote
   memory.  Returns true if all went well, false otherwise.  */

static bool
jit_read_descriptor (gdbarch *gdbarch,
		     jit_descriptor *descriptor,
		     objfile *jiter)
{
  int err;
  struct type *ptr_type;
  int ptr_size;
  int desc_size;
  gdb_byte *desc_buf;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  gdb_assert (jiter != nullptr);
  jiter_objfile_data *objf_data = jiter->jiter_data.get ();
  gdb_assert (objf_data != nullptr);

  CORE_ADDR addr = MSYMBOL_VALUE_ADDRESS (jiter, objf_data->descriptor);

  if (jit_debug)
    fprintf_unfiltered (gdb_stdlog,
			"jit_read_descriptor, descriptor_addr = %s\n",
			paddress (gdbarch, addr));

  /* Figure out how big the descriptor is on the remote and how to read it.  */
  ptr_type = builtin_type (gdbarch)->builtin_data_ptr;
  ptr_size = TYPE_LENGTH (ptr_type);
  desc_size = 8 + 2 * ptr_size;  /* Two 32-bit ints and two pointers.  */
  desc_buf = (gdb_byte *) alloca (desc_size);

  /* Read the descriptor.  */
  err = target_read_memory (addr, desc_buf, desc_size);
  if (err)
    {
      printf_unfiltered (_("Unable to read JIT descriptor from "
			   "remote memory\n"));
      return false;
    }

  /* Fix the endianness to match the host.  */
  descriptor->version = extract_unsigned_integer (&desc_buf[0], 4, byte_order);
  descriptor->action_flag =
      extract_unsigned_integer (&desc_buf[4], 4, byte_order);
  descriptor->relevant_entry = extract_typed_address (&desc_buf[8], ptr_type);
  descriptor->first_entry =
      extract_typed_address (&desc_buf[8 + ptr_size], ptr_type);

  return true;
}

/* Helper function for reading a JITed code entry from remote memory.  */

static void
jit_read_code_entry (struct gdbarch *gdbarch,
		     CORE_ADDR code_addr, struct jit_code_entry *code_entry)
{
  int err, off;
  struct type *ptr_type;
  int ptr_size;
  int entry_size;
  int align_bytes;
  gdb_byte *entry_buf;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  /* Figure out how big the entry is on the remote and how to read it.  */
  ptr_type = builtin_type (gdbarch)->builtin_data_ptr;
  ptr_size = TYPE_LENGTH (ptr_type);

  /* Figure out where the uint64_t value will be.  */
  align_bytes = type_align (builtin_type (gdbarch)->builtin_uint64);
  off = 3 * ptr_size;
  off = (off + (align_bytes - 1)) & ~(align_bytes - 1);

  entry_size = off + 8;  /* Three pointers and one 64-bit int.  */
  entry_buf = (gdb_byte *) alloca (entry_size);

  /* Read the entry.  */
  err = target_read_memory (code_addr, entry_buf, entry_size);
  if (err)
    error (_("Unable to read JIT code entry from remote memory!"));

  /* Fix the endianness to match the host.  */
  ptr_type = builtin_type (gdbarch)->builtin_data_ptr;
  code_entry->next_entry = extract_typed_address (&entry_buf[0], ptr_type);
  code_entry->prev_entry =
      extract_typed_address (&entry_buf[ptr_size], ptr_type);
  code_entry->symfile_addr =
      extract_typed_address (&entry_buf[2 * ptr_size], ptr_type);
  code_entry->symfile_size =
      extract_unsigned_integer (&entry_buf[off], 8, byte_order);
}

/* Proxy object for building a block.  */

struct gdb_block
{
  gdb_block (gdb_block *parent, CORE_ADDR begin, CORE_ADDR end,
	     const char *name)
    : parent (parent),
      begin (begin),
      end (end),
      name (name != nullptr ? xstrdup (name) : nullptr)
  {}

  /* The parent of this block.  */
  struct gdb_block *parent;

  /* Points to the "real" block that is being built out of this
     instance.  This block will be added to a blockvector, which will
     then be added to a symtab.  */
  struct block *real_block = nullptr;

  /* The first and last code address corresponding to this block.  */
  CORE_ADDR begin, end;

  /* The name of this block (if any).  If this is non-NULL, the
     FUNCTION symbol symbol is set to this value.  */
  gdb::unique_xmalloc_ptr<char> name;
};

/* Proxy object for building a symtab.  */

struct gdb_symtab
{
  explicit gdb_symtab (const char *file_name)
    : file_name (file_name != nullptr ? file_name : "")
  {}

  /* The list of blocks in this symtab.  These will eventually be
     converted to real blocks.

     This is specifically a linked list, instead of, for example, a vector,
     because the pointers are returned to the user's debug info reader.  So
     it's important that the objects don't change location during their
     lifetime (which would happen with a vector of objects getting resized).  */
  std::forward_list<gdb_block> blocks;

  /* The number of blocks inserted.  */
  int nblocks = 0;

  /* A mapping between line numbers to PC.  */
  gdb::unique_xmalloc_ptr<struct linetable> linetable;

  /* The source file for this symtab.  */
  std::string file_name;
};

/* Proxy object for building an object.  */

struct gdb_object
{
  /* Symtabs of this object.

     This is specifically a linked list, instead of, for example, a vector,
     because the pointers are returned to the user's debug info reader.  So
     it's important that the objects don't change location during their
     lifetime (which would happen with a vector of objects getting resized).  */
  std::forward_list<gdb_symtab> symtabs;
};

/* The type of the `private' data passed around by the callback
   functions.  */

typedef CORE_ADDR jit_dbg_reader_data;

/* The reader calls into this function to read data off the targets
   address space.  */

static enum gdb_status
jit_target_read_impl (GDB_CORE_ADDR target_mem, void *gdb_buf, int len)
{
  int result = target_read_memory ((CORE_ADDR) target_mem,
				   (gdb_byte *) gdb_buf, len);
  if (result == 0)
    return GDB_SUCCESS;
  else
    return GDB_FAIL;
}

/* The reader calls into this function to create a new gdb_object
   which it can then pass around to the other callbacks.  Right now,
   all that is required is allocating the memory.  */

static struct gdb_object *
jit_object_open_impl (struct gdb_symbol_callbacks *cb)
{
  /* CB is not required right now, but sometime in the future we might
     need a handle to it, and we'd like to do that without breaking
     the ABI.  */
  return new gdb_object;
}

/* Readers call into this function to open a new gdb_symtab, which,
   again, is passed around to other callbacks.  */

static struct gdb_symtab *
jit_symtab_open_impl (struct gdb_symbol_callbacks *cb,
		      struct gdb_object *object,
		      const char *file_name)
{
  /* CB stays unused.  See comment in jit_object_open_impl.  */

  object->symtabs.emplace_front (file_name);
  return &object->symtabs.front ();
}

/* Called by readers to open a new gdb_block.  This function also
   inserts the new gdb_block in the correct place in the corresponding
   gdb_symtab.  */

static struct gdb_block *
jit_block_open_impl (struct gdb_symbol_callbacks *cb,
		     struct gdb_symtab *symtab, struct gdb_block *parent,
		     GDB_CORE_ADDR begin, GDB_CORE_ADDR end, const char *name)
{
  /* Place the block at the beginning of the list, it will be sorted when the
     symtab is finalized.  */
  symtab->blocks.emplace_front (parent, begin, end, name);
  symtab->nblocks++;

  return &symtab->blocks.front ();
}

/* Readers call this to add a line mapping (from PC to line number) to
   a gdb_symtab.  */

static void
jit_symtab_line_mapping_add_impl (struct gdb_symbol_callbacks *cb,
				  struct gdb_symtab *stab, int nlines,
				  struct gdb_line_mapping *map)
{
  int i;
  int alloc_len;

  if (nlines < 1)
    return;

  alloc_len = sizeof (struct linetable)
	      + (nlines - 1) * sizeof (struct linetable_entry);
  stab->linetable.reset (XNEWVAR (struct linetable, alloc_len));
  stab->linetable->nitems = nlines;
  for (i = 0; i < nlines; i++)
    {
      stab->linetable->item[i].pc = (CORE_ADDR) map[i].pc;
      stab->linetable->item[i].line = map[i].line;
      stab->linetable->item[i].is_stmt = 1;
    }
}

/* Called by readers to close a gdb_symtab.  Does not need to do
   anything as of now.  */

static void
jit_symtab_close_impl (struct gdb_symbol_callbacks *cb,
		       struct gdb_symtab *stab)
{
  /* Right now nothing needs to be done here.  We may need to do some
     cleanup here in the future (again, without breaking the plugin
     ABI).  */
}

/* Transform STAB to a proper symtab, and add it it OBJFILE.  */

static void
finalize_symtab (struct gdb_symtab *stab, struct objfile *objfile)
{
  struct compunit_symtab *cust;
  size_t blockvector_size;
  CORE_ADDR begin, end;
  struct blockvector *bv;

  int actual_nblocks = FIRST_LOCAL_BLOCK + stab->nblocks;

  /* Sort the blocks in the order they should appear in the blockvector.  */
  stab->blocks.sort([] (const gdb_block &a, const gdb_block &b)
    {
      if (a.begin != b.begin)
	return a.begin < b.begin;

      return a.end > b.end;
    });

  cust = allocate_compunit_symtab (objfile, stab->file_name.c_str ());
  allocate_symtab (cust, stab->file_name.c_str ());
  add_compunit_symtab_to_objfile (cust);

  /* JIT compilers compile in memory.  */
  COMPUNIT_DIRNAME (cust) = NULL;

  /* Copy over the linetable entry if one was provided.  */
  if (stab->linetable)
    {
      size_t size = ((stab->linetable->nitems - 1)
		     * sizeof (struct linetable_entry)
		     + sizeof (struct linetable));
      SYMTAB_LINETABLE (COMPUNIT_FILETABS (cust))
	= (struct linetable *) obstack_alloc (&objfile->objfile_obstack, size);
      memcpy (SYMTAB_LINETABLE (COMPUNIT_FILETABS (cust)),
	      stab->linetable.get (), size);
    }

  blockvector_size = (sizeof (struct blockvector)
		      + (actual_nblocks - 1) * sizeof (struct block *));
  bv = (struct blockvector *) obstack_alloc (&objfile->objfile_obstack,
					     blockvector_size);
  COMPUNIT_BLOCKVECTOR (cust) = bv;

  /* At the end of this function, (begin, end) will contain the PC range this
     entire blockvector spans.  */
  BLOCKVECTOR_MAP (bv) = NULL;
  begin = stab->blocks.front ().begin;
  end = stab->blocks.front ().end;
  BLOCKVECTOR_NBLOCKS (bv) = actual_nblocks;

  /* First run over all the gdb_block objects, creating a real block
     object for each.  Simultaneously, keep setting the real_block
     fields.  */
  int block_idx = FIRST_LOCAL_BLOCK;
  for (gdb_block &gdb_block_iter : stab->blocks)
    {
      struct block *new_block = allocate_block (&objfile->objfile_obstack);
      struct symbol *block_name = new (&objfile->objfile_obstack) symbol;
      struct type *block_type = arch_type (objfile->arch (),
					   TYPE_CODE_VOID,
					   TARGET_CHAR_BIT,
					   "void");

      BLOCK_MULTIDICT (new_block)
	= mdict_create_linear (&objfile->objfile_obstack, NULL);
      /* The address range.  */
      BLOCK_START (new_block) = (CORE_ADDR) gdb_block_iter.begin;
      BLOCK_END (new_block) = (CORE_ADDR) gdb_block_iter.end;

      /* The name.  */
      SYMBOL_DOMAIN (block_name) = VAR_DOMAIN;
      SYMBOL_ACLASS_INDEX (block_name) = LOC_BLOCK;
      symbol_set_symtab (block_name, COMPUNIT_FILETABS (cust));
      SYMBOL_TYPE (block_name) = lookup_function_type (block_type);
      SYMBOL_BLOCK_VALUE (block_name) = new_block;

      block_name->m_name = obstack_strdup (&objfile->objfile_obstack,
					   gdb_block_iter.name.get ());

      BLOCK_FUNCTION (new_block) = block_name;

      BLOCKVECTOR_BLOCK (bv, block_idx) = new_block;
      if (begin > BLOCK_START (new_block))
	begin = BLOCK_START (new_block);
      if (end < BLOCK_END (new_block))
	end = BLOCK_END (new_block);

      gdb_block_iter.real_block = new_block;

      block_idx++;
    }

  /* Now add the special blocks.  */
  struct block *block_iter = NULL;
  for (enum block_enum i : { GLOBAL_BLOCK, STATIC_BLOCK })
    {
      struct block *new_block;

      new_block = (i == GLOBAL_BLOCK
		   ? allocate_global_block (&objfile->objfile_obstack)
		   : allocate_block (&objfile->objfile_obstack));
      BLOCK_MULTIDICT (new_block)
	= mdict_create_linear (&objfile->objfile_obstack, NULL);
      BLOCK_SUPERBLOCK (new_block) = block_iter;
      block_iter = new_block;

      BLOCK_START (new_block) = (CORE_ADDR) begin;
      BLOCK_END (new_block) = (CORE_ADDR) end;

      BLOCKVECTOR_BLOCK (bv, i) = new_block;

      if (i == GLOBAL_BLOCK)
	set_block_compunit_symtab (new_block, cust);
    }

  /* Fill up the superblock fields for the real blocks, using the
     real_block fields populated earlier.  */
  for (gdb_block &gdb_block_iter : stab->blocks)
    {
      if (gdb_block_iter.parent != NULL)
	{
	  /* If the plugin specifically mentioned a parent block, we
	     use that.  */
	  BLOCK_SUPERBLOCK (gdb_block_iter.real_block) =
	    gdb_block_iter.parent->real_block;
	}
      else
	{
	  /* And if not, we set a default parent block.  */
	  BLOCK_SUPERBLOCK (gdb_block_iter.real_block) =
	    BLOCKVECTOR_BLOCK (bv, STATIC_BLOCK);
	}
    }
}

/* Called when closing a gdb_objfile.  Converts OBJ to a proper
   objfile.  */

static void
jit_object_close_impl (struct gdb_symbol_callbacks *cb,
		       struct gdb_object *obj)
{
  struct objfile *objfile;
  jit_dbg_reader_data *priv_data;

  priv_data = (jit_dbg_reader_data *) cb->priv_data;

  objfile = objfile::make (nullptr, "<< JIT compiled code >>",
			   OBJF_NOT_FILENAME);
  objfile->per_bfd->gdbarch = target_gdbarch ();

  for (gdb_symtab &symtab : obj->symtabs)
    finalize_symtab (&symtab, objfile);

  add_objfile_entry (objfile, *priv_data);

  delete obj;
}

/* Try to read CODE_ENTRY using the loaded jit reader (if any).
   ENTRY_ADDR is the address of the struct jit_code_entry in the
   inferior address space.  */

static int
jit_reader_try_read_symtab (struct jit_code_entry *code_entry,
			    CORE_ADDR entry_addr)
{
  int status;
  jit_dbg_reader_data priv_data;
  struct gdb_reader_funcs *funcs;
  struct gdb_symbol_callbacks callbacks =
    {
      jit_object_open_impl,
      jit_symtab_open_impl,
      jit_block_open_impl,
      jit_symtab_close_impl,
      jit_object_close_impl,

      jit_symtab_line_mapping_add_impl,
      jit_target_read_impl,

      &priv_data
    };

  priv_data = entry_addr;

  if (!loaded_jit_reader)
    return 0;

  gdb::byte_vector gdb_mem (code_entry->symfile_size);

  status = 1;
  try
    {
      if (target_read_memory (code_entry->symfile_addr, gdb_mem.data (),
			      code_entry->symfile_size))
	status = 0;
    }
  catch (const gdb_exception &e)
    {
      status = 0;
    }

  if (status)
    {
      funcs = loaded_jit_reader->functions;
      if (funcs->read (funcs, &callbacks, gdb_mem.data (),
		       code_entry->symfile_size)
	  != GDB_SUCCESS)
	status = 0;
    }

  if (jit_debug && status == 0)
    fprintf_unfiltered (gdb_stdlog,
			"Could not read symtab using the loaded JIT reader.\n");
  return status;
}

/* Try to read CODE_ENTRY using BFD.  ENTRY_ADDR is the address of the
   struct jit_code_entry in the inferior address space.  */

static void
jit_bfd_try_read_symtab (struct jit_code_entry *code_entry,
			 CORE_ADDR entry_addr,
			 struct gdbarch *gdbarch)
{
  struct bfd_section *sec;
  struct objfile *objfile;
  const struct bfd_arch_info *b;

  if (jit_debug)
    fprintf_unfiltered (gdb_stdlog,
			"jit_bfd_try_read_symtab, symfile_addr = %s, "
			"symfile_size = %s\n",
			paddress (gdbarch, code_entry->symfile_addr),
			pulongest (code_entry->symfile_size));

  gdb_bfd_ref_ptr nbfd (bfd_open_from_target_memory (code_entry->symfile_addr,
						     code_entry->symfile_size,
						     gnutarget));
  if (nbfd == NULL)
    {
      puts_unfiltered (_("Error opening JITed symbol file, ignoring it.\n"));
      return;
    }

  /* Check the format.  NOTE: This initializes important data that GDB uses!
     We would segfault later without this line.  */
  if (!bfd_check_format (nbfd.get (), bfd_object))
    {
      printf_unfiltered (_("\
JITed symbol file is not an object file, ignoring it.\n"));
      return;
    }

  /* Check bfd arch.  */
  b = gdbarch_bfd_arch_info (gdbarch);
  if (b->compatible (b, bfd_get_arch_info (nbfd.get ())) != b)
    warning (_("JITed object file architecture %s is not compatible "
	       "with target architecture %s."),
	     bfd_get_arch_info (nbfd.get ())->printable_name,
	     b->printable_name);

  /* Read the section address information out of the symbol file.  Since the
     file is generated by the JIT at runtime, it should all of the absolute
     addresses that we care about.  */
  section_addr_info sai;
  for (sec = nbfd->sections; sec != NULL; sec = sec->next)
    if ((bfd_section_flags (sec) & (SEC_ALLOC|SEC_LOAD)) != 0)
      {
	/* We assume that these virtual addresses are absolute, and do not
	   treat them as offsets.  */
	sai.emplace_back (bfd_section_vma (sec),
			  bfd_section_name (sec),
			  sec->index);
      }

  /* This call does not take ownership of SAI.  */
  objfile = symbol_file_add_from_bfd (nbfd.get (),
				      bfd_get_filename (nbfd.get ()), 0,
				      &sai,
				      OBJF_SHARED | OBJF_NOT_FILENAME, NULL);

  add_objfile_entry (objfile, entry_addr);
}

/* This function registers code associated with a JIT code entry.  It uses the
   pointer and size pair in the entry to read the symbol file from the remote
   and then calls symbol_file_add_from_local_memory to add it as though it were
   a symbol file added by the user.  */

static void
jit_register_code (struct gdbarch *gdbarch,
		   CORE_ADDR entry_addr, struct jit_code_entry *code_entry)
{
  int success;

  if (jit_debug)
    fprintf_unfiltered (gdb_stdlog,
			"jit_register_code, symfile_addr = %s, "
			"symfile_size = %s\n",
			paddress (gdbarch, code_entry->symfile_addr),
			pulongest (code_entry->symfile_size));

  success = jit_reader_try_read_symtab (code_entry, entry_addr);

  if (!success)
    jit_bfd_try_read_symtab (code_entry, entry_addr, gdbarch);
}

/* Look up the objfile with this code entry address.  */

static struct objfile *
jit_find_objf_with_entry_addr (CORE_ADDR entry_addr)
{
  for (objfile *objf : current_program_space->objfiles ())
    {
      if (objf->jited_data != nullptr && objf->jited_data->addr == entry_addr)
	return objf;
    }

  return NULL;
}

/* This is called when a breakpoint is deleted.  It updates the
   inferior's cache, if needed.  */

static void
jit_breakpoint_deleted (struct breakpoint *b)
{
  if (b->type != bp_jit_event)
    return;

  for (bp_location *iter = b->loc; iter != nullptr; iter = iter->next)
    {
      for (objfile *objf : iter->pspace->objfiles ())
	{
	  jiter_objfile_data *jiter_data = objf->jiter_data.get ();

	  if (jiter_data != nullptr
	      && jiter_data->jit_breakpoint == iter->owner)
	    {
	      jiter_data->cached_code_address = 0;
	      jiter_data->jit_breakpoint = nullptr;
	    }
	}
    }
}

/* (Re-)Initialize the jit breakpoints for JIT-producing objfiles in
   PSPACE.  */

static void
jit_breakpoint_re_set_internal (struct gdbarch *gdbarch, program_space *pspace)
{
  for (objfile *the_objfile : pspace->objfiles ())
    {
      if (the_objfile->skip_jit_symbol_lookup)
	continue;

      /* Lookup the registration symbol.  If it is missing, then we
	 assume we are not attached to a JIT.  */
      bound_minimal_symbol reg_symbol
	= lookup_minimal_symbol (jit_break_name, nullptr, the_objfile);
      if (reg_symbol.minsym == NULL
	  || BMSYMBOL_VALUE_ADDRESS (reg_symbol) == 0)
	{
	  /* No need to repeat the lookup the next time.  */
	  the_objfile->skip_jit_symbol_lookup = true;
	  continue;
	}

      bound_minimal_symbol desc_symbol
	= lookup_minimal_symbol (jit_descriptor_name, NULL, the_objfile);
      if (desc_symbol.minsym == NULL
	  || BMSYMBOL_VALUE_ADDRESS (desc_symbol) == 0)
	{
	  /* No need to repeat the lookup the next time.  */
	  the_objfile->skip_jit_symbol_lookup = true;
	  continue;
	}

      jiter_objfile_data *objf_data
	= get_jiter_objfile_data (reg_symbol.objfile);
      objf_data->register_code = reg_symbol.minsym;
      objf_data->descriptor = desc_symbol.minsym;

      CORE_ADDR addr = MSYMBOL_VALUE_ADDRESS (the_objfile,
					      objf_data->register_code);

      if (jit_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "jit_breakpoint_re_set_internal, "
			    "breakpoint_addr = %s\n",
			    paddress (gdbarch, addr));

      /* Check if we need to re-create the breakpoint.  */
      if (objf_data->cached_code_address == addr)
	continue;

      /* Delete the old breakpoint.  */
      if (objf_data->jit_breakpoint != nullptr)
	delete_breakpoint (objf_data->jit_breakpoint);

      /* Put a breakpoint in the registration symbol.  */
      objf_data->cached_code_address = addr;
      objf_data->jit_breakpoint = create_jit_event_breakpoint (gdbarch, addr);
    }
}

/* The private data passed around in the frame unwind callback
   functions.  */

struct jit_unwind_private
{
  /* Cached register values.  See jit_frame_sniffer to see how this
     works.  */
  detached_regcache *regcache;

  /* The frame being unwound.  */
  struct frame_info *this_frame;
};

/* Sets the value of a particular register in this frame.  */

static void
jit_unwind_reg_set_impl (struct gdb_unwind_callbacks *cb, int dwarf_regnum,
			 struct gdb_reg_value *value)
{
  struct jit_unwind_private *priv;
  int gdb_reg;

  priv = (struct jit_unwind_private *) cb->priv_data;

  gdb_reg = gdbarch_dwarf2_reg_to_regnum (get_frame_arch (priv->this_frame),
					  dwarf_regnum);
  if (gdb_reg == -1)
    {
      if (jit_debug)
	fprintf_unfiltered (gdb_stdlog,
			    _("Could not recognize DWARF regnum %d"),
			    dwarf_regnum);
      value->free (value);
      return;
    }

  priv->regcache->raw_supply (gdb_reg, value->value);
  value->free (value);
}

static void
reg_value_free_impl (struct gdb_reg_value *value)
{
  xfree (value);
}

/* Get the value of register REGNUM in the previous frame.  */

static struct gdb_reg_value *
jit_unwind_reg_get_impl (struct gdb_unwind_callbacks *cb, int regnum)
{
  struct jit_unwind_private *priv;
  struct gdb_reg_value *value;
  int gdb_reg, size;
  struct gdbarch *frame_arch;

  priv = (struct jit_unwind_private *) cb->priv_data;
  frame_arch = get_frame_arch (priv->this_frame);

  gdb_reg = gdbarch_dwarf2_reg_to_regnum (frame_arch, regnum);
  size = register_size (frame_arch, gdb_reg);
  value = ((struct gdb_reg_value *)
	   xmalloc (sizeof (struct gdb_reg_value) + size - 1));
  value->defined = deprecated_frame_register_read (priv->this_frame, gdb_reg,
						   value->value);
  value->size = size;
  value->free = reg_value_free_impl;
  return value;
}

/* gdb_reg_value has a free function, which must be called on each
   saved register value.  */

static void
jit_dealloc_cache (struct frame_info *this_frame, void *cache)
{
  struct jit_unwind_private *priv_data = (struct jit_unwind_private *) cache;

  gdb_assert (priv_data->regcache != NULL);
  delete priv_data->regcache;
  xfree (priv_data);
}

/* The frame sniffer for the pseudo unwinder.

   While this is nominally a frame sniffer, in the case where the JIT
   reader actually recognizes the frame, it does a lot more work -- it
   unwinds the frame and saves the corresponding register values in
   the cache.  jit_frame_prev_register simply returns the saved
   register values.  */

static int
jit_frame_sniffer (const struct frame_unwind *self,
		   struct frame_info *this_frame, void **cache)
{
  struct jit_unwind_private *priv_data;
  struct gdb_unwind_callbacks callbacks;
  struct gdb_reader_funcs *funcs;

  callbacks.reg_get = jit_unwind_reg_get_impl;
  callbacks.reg_set = jit_unwind_reg_set_impl;
  callbacks.target_read = jit_target_read_impl;

  if (loaded_jit_reader == NULL)
    return 0;

  funcs = loaded_jit_reader->functions;

  gdb_assert (!*cache);

  *cache = XCNEW (struct jit_unwind_private);
  priv_data = (struct jit_unwind_private *) *cache;
  /* Take a snapshot of current regcache.  */
  priv_data->regcache = new detached_regcache (get_frame_arch (this_frame),
					       true);
  priv_data->this_frame = this_frame;

  callbacks.priv_data = priv_data;

  /* Try to coax the provided unwinder to unwind the stack */
  if (funcs->unwind (funcs, &callbacks) == GDB_SUCCESS)
    {
      if (jit_debug)
	fprintf_unfiltered (gdb_stdlog, _("Successfully unwound frame using "
					  "JIT reader.\n"));
      return 1;
    }
  if (jit_debug)
    fprintf_unfiltered (gdb_stdlog, _("Could not unwind frame using "
				      "JIT reader.\n"));

  jit_dealloc_cache (this_frame, *cache);
  *cache = NULL;

  return 0;
}


/* The frame_id function for the pseudo unwinder.  Relays the call to
   the loaded plugin.  */

static void
jit_frame_this_id (struct frame_info *this_frame, void **cache,
		   struct frame_id *this_id)
{
  struct jit_unwind_private priv;
  struct gdb_frame_id frame_id;
  struct gdb_reader_funcs *funcs;
  struct gdb_unwind_callbacks callbacks;

  priv.regcache = NULL;
  priv.this_frame = this_frame;

  /* We don't expect the frame_id function to set any registers, so we
     set reg_set to NULL.  */
  callbacks.reg_get = jit_unwind_reg_get_impl;
  callbacks.reg_set = NULL;
  callbacks.target_read = jit_target_read_impl;
  callbacks.priv_data = &priv;

  gdb_assert (loaded_jit_reader);
  funcs = loaded_jit_reader->functions;

  frame_id = funcs->get_frame_id (funcs, &callbacks);
  *this_id = frame_id_build (frame_id.stack_address, frame_id.code_address);
}

/* Pseudo unwinder function.  Reads the previously fetched value for
   the register from the cache.  */

static struct value *
jit_frame_prev_register (struct frame_info *this_frame, void **cache, int reg)
{
  struct jit_unwind_private *priv = (struct jit_unwind_private *) *cache;
  struct gdbarch *gdbarch;

  if (priv == NULL)
    return frame_unwind_got_optimized (this_frame, reg);

  gdbarch = priv->regcache->arch ();
  gdb_byte *buf = (gdb_byte *) alloca (register_size (gdbarch, reg));
  enum register_status status = priv->regcache->cooked_read (reg, buf);

  if (status == REG_VALID)
    return frame_unwind_got_bytes (this_frame, reg, buf);
  else
    return frame_unwind_got_optimized (this_frame, reg);
}

/* Relay everything back to the unwinder registered by the JIT debug
   info reader.*/

static const struct frame_unwind jit_frame_unwind =
{
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  jit_frame_this_id,
  jit_frame_prev_register,
  NULL,
  jit_frame_sniffer,
  jit_dealloc_cache
};


/* This is the information that is stored at jit_gdbarch_data for each
   architecture.  */

struct jit_gdbarch_data_type
{
  /* Has the (pseudo) unwinder been prepended? */
  int unwinder_registered;
};

/* Check GDBARCH and prepend the pseudo JIT unwinder if needed.  */

static void
jit_prepend_unwinder (struct gdbarch *gdbarch)
{
  struct jit_gdbarch_data_type *data;

  data
    = (struct jit_gdbarch_data_type *) gdbarch_data (gdbarch, jit_gdbarch_data);
  if (!data->unwinder_registered)
    {
      frame_unwind_prepend_unwinder (gdbarch, &jit_frame_unwind);
      data->unwinder_registered = 1;
    }
}

/* Register any already created translations.  */

static void
jit_inferior_init (struct gdbarch *gdbarch)
{
  struct jit_descriptor descriptor;
  struct jit_code_entry cur_entry;
  CORE_ADDR cur_entry_addr;

  if (jit_debug)
    fprintf_unfiltered (gdb_stdlog, "jit_inferior_init\n");

  jit_prepend_unwinder (gdbarch);

  jit_breakpoint_re_set_internal (gdbarch, current_program_space);

  for (objfile *jiter : current_program_space->objfiles ())
    {
      if (jiter->jiter_data == nullptr)
	continue;

      /* Read the descriptor so we can check the version number and load
	 any already JITed functions.  */
      if (!jit_read_descriptor (gdbarch, &descriptor, jiter))
	continue;

      /* Check that the version number agrees with that we support.  */
      if (descriptor.version != 1)
	{
	  printf_unfiltered (_("Unsupported JIT protocol version %ld "
			       "in descriptor (expected 1)\n"),
			     (long) descriptor.version);
	  continue;
	}

      /* If we've attached to a running program, we need to check the
	 descriptor to register any functions that were already
	 generated.  */
      for (cur_entry_addr = descriptor.first_entry;
	   cur_entry_addr != 0;
	   cur_entry_addr = cur_entry.next_entry)
	{
	  jit_read_code_entry (gdbarch, cur_entry_addr, &cur_entry);

	  /* This hook may be called many times during setup, so make sure
	     we don't add the same symbol file twice.  */
	  if (jit_find_objf_with_entry_addr (cur_entry_addr) != NULL)
	    continue;

	  jit_register_code (gdbarch, cur_entry_addr, &cur_entry);
	}
    }
}

/* inferior_created observer.  */

static void
jit_inferior_created (struct target_ops *ops, int from_tty)
{
  jit_inferior_created_hook ();
}

/* Exported routine to call when an inferior has been created.  */

void
jit_inferior_created_hook (void)
{
  jit_inferior_init (target_gdbarch ());
}

/* Exported routine to call to re-set the jit breakpoints,
   e.g. when a program is rerun.  */

void
jit_breakpoint_re_set (void)
{
  jit_breakpoint_re_set_internal (target_gdbarch (), current_program_space);
}

/* This function cleans up any code entries left over when the
   inferior exits.  We get left over code when the inferior exits
   without unregistering its code, for example when it crashes.  */

static void
jit_inferior_exit_hook (struct inferior *inf)
{
  for (objfile *objf : current_program_space->objfiles_safe ())
    {
      if (objf->jited_data != nullptr && objf->jited_data->addr != 0)
	objf->unlink ();
    }
}

void
jit_event_handler (gdbarch *gdbarch, objfile *jiter)
{
  struct jit_descriptor descriptor;

  /* If we get a JIT breakpoint event for this objfile, it is necessarily a
     JITer.  */
  gdb_assert (jiter->jiter_data != nullptr);

  /* Read the descriptor from remote memory.  */
  if (!jit_read_descriptor (gdbarch, &descriptor, jiter))
    return;
  CORE_ADDR entry_addr = descriptor.relevant_entry;

  /* Do the corresponding action.  */
  switch (descriptor.action_flag)
    {
    case JIT_NOACTION:
      break;

    case JIT_REGISTER:
      {
	jit_code_entry code_entry;
	jit_read_code_entry (gdbarch, entry_addr, &code_entry);
	jit_register_code (gdbarch, entry_addr, &code_entry);
	break;
      }

    case JIT_UNREGISTER:
      {
	objfile *jited = jit_find_objf_with_entry_addr (entry_addr);
	if (jited == nullptr)
	  printf_unfiltered (_("Unable to find JITed code "
			       "entry at address: %s\n"),
			     paddress (gdbarch, entry_addr));
	else
	  jited->unlink ();

	break;
      }

    default:
      error (_("Unknown action_flag value in JIT descriptor!"));
      break;
    }
}

/* Initialize the jit_gdbarch_data slot with an instance of struct
   jit_gdbarch_data_type */

static void *
jit_gdbarch_data_init (struct obstack *obstack)
{
  struct jit_gdbarch_data_type *data =
    XOBNEW (obstack, struct jit_gdbarch_data_type);

  data->unwinder_registered = 0;

  return data;
}

void _initialize_jit ();
void
_initialize_jit ()
{
  jit_reader_dir = relocate_gdb_directory (JIT_READER_DIR,
					   JIT_READER_DIR_RELOCATABLE);
  add_setshow_zuinteger_cmd ("jit", class_maintenance, &jit_debug,
			     _("Set JIT debugging."),
			     _("Show JIT debugging."),
			     _("When non-zero, JIT debugging is enabled."),
			     NULL,
			     show_jit_debug,
			     &setdebuglist, &showdebuglist);

  gdb::observers::inferior_created.attach (jit_inferior_created);
  gdb::observers::inferior_exit.attach (jit_inferior_exit_hook);
  gdb::observers::breakpoint_deleted.attach (jit_breakpoint_deleted);

  jit_gdbarch_data = gdbarch_data_register_pre_init (jit_gdbarch_data_init);
  if (is_dl_available ())
    {
      struct cmd_list_element *c;

      c = add_com ("jit-reader-load", no_class, jit_reader_load_command, _("\
Load FILE as debug info reader and unwinder for JIT compiled code.\n\
Usage: jit-reader-load FILE\n\
Try to load file FILE as a debug info reader (and unwinder) for\n\
JIT compiled code.  The file is loaded from " JIT_READER_DIR ",\n\
relocated relative to the GDB executable if required."));
      set_cmd_completer (c, filename_completer);

      c = add_com ("jit-reader-unload", no_class,
		   jit_reader_unload_command, _("\
Unload the currently loaded JIT debug info reader.\n\
Usage: jit-reader-unload\n\n\
Do \"help jit-reader-load\" for info on loading debug info readers."));
      set_cmd_completer (c, noop_completer);
    }
}
