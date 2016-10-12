/* Handle JIT code generation in the inferior for GDB, the GNU Debugger.

   Copyright (C) 2009-2016 Free Software Foundation, Inc.

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
#include "observer.h"
#include "objfiles.h"
#include "regcache.h"
#include "symfile.h"
#include "symtab.h"
#include "target.h"
#include "gdb-dlfcn.h"
#include <sys/stat.h>
#include "gdb_bfd.h"
#include "readline/tilde.h"
#include "completer.h"

static const char *jit_reader_dir = NULL;

static const struct objfile_data *jit_objfile_data;

static const char *const jit_break_name = "__jit_debug_register_code";

static const char *const jit_descriptor_name = "__jit_debug_descriptor";

static const struct program_space_data *jit_program_space_data = NULL;

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

/* Openning the file is a no-op.  */

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

static struct bfd *
bfd_open_from_target_memory (CORE_ADDR addr, ULONGEST size, char *target)
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

/* One reader that has been loaded successfully, and can potentially be used to
   parse debug info.  */

static struct jit_reader
{
  struct gdb_reader_funcs *functions;
  void *handle;
} *loaded_jit_reader = NULL;

typedef struct gdb_reader_funcs * (reader_init_fn_type) (void);
static const char *reader_init_fn_sym = "gdb_init_reader";

/* Try to load FILE_NAME as a JIT debug info reader.  */

static struct jit_reader *
jit_reader_load (const char *file_name)
{
  void *so;
  reader_init_fn_type *init_fn;
  struct jit_reader *new_reader = NULL;
  struct gdb_reader_funcs *funcs = NULL;
  struct cleanup *old_cleanups;

  if (jit_debug)
    fprintf_unfiltered (gdb_stdlog, _("Opening shared object %s.\n"),
                        file_name);
  so = gdb_dlopen (file_name);
  old_cleanups = make_cleanup_dlclose (so);

  init_fn = (reader_init_fn_type *) gdb_dlsym (so, reader_init_fn_sym);
  if (!init_fn)
    error (_("Could not locate initialization function: %s."),
          reader_init_fn_sym);

  if (gdb_dlsym (so, "plugin_is_GPL_compatible") == NULL)
    error (_("Reader not GPL compatible."));

  funcs = init_fn ();
  if (funcs->reader_version != GDB_READER_INTERFACE_VERSION)
    error (_("Reader version does not match GDB version."));

  new_reader = XCNEW (struct jit_reader);
  new_reader->functions = funcs;
  new_reader->handle = so;

  discard_cleanups (old_cleanups);
  return new_reader;
}

/* Provides the jit-reader-load command.  */

static void
jit_reader_load_command (char *args, int from_tty)
{
  char *so_name;
  struct cleanup *prev_cleanup;

  if (args == NULL)
    error (_("No reader name provided."));
  args = tilde_expand (args);
  prev_cleanup = make_cleanup (xfree, args);

  if (loaded_jit_reader != NULL)
    error (_("JIT reader already loaded.  Run jit-reader-unload first."));

  if (IS_ABSOLUTE_PATH (args))
    so_name = args;
  else
    {
      so_name = xstrprintf ("%s%s%s", jit_reader_dir, SLASH_STRING, args);
      make_cleanup (xfree, so_name);
    }

  loaded_jit_reader = jit_reader_load (so_name);
  reinit_frame_cache ();
  jit_inferior_created_hook ();
  do_cleanups (prev_cleanup);
}

/* Provides the jit-reader-unload command.  */

static void
jit_reader_unload_command (char *args, int from_tty)
{
  if (!loaded_jit_reader)
    error (_("No JIT reader loaded."));

  reinit_frame_cache ();
  jit_inferior_exit_hook (current_inferior ());
  loaded_jit_reader->functions->destroy (loaded_jit_reader->functions);

  gdb_dlclose (loaded_jit_reader->handle);
  xfree (loaded_jit_reader);
  loaded_jit_reader = NULL;
}

/* Per-program space structure recording which objfile has the JIT
   symbols.  */

struct jit_program_space_data
{
  /* The objfile.  This is NULL if no objfile holds the JIT
     symbols.  */

  struct objfile *objfile;

  /* If this program space has __jit_debug_register_code, this is the
     cached address from the minimal symbol.  This is used to detect
     relocations requiring the breakpoint to be re-created.  */

  CORE_ADDR cached_code_address;

  /* This is the JIT event breakpoint, or NULL if it has not been
     set.  */

  struct breakpoint *jit_breakpoint;
};

/* Per-objfile structure recording the addresses in the program space.
   This object serves two purposes: for ordinary objfiles, it may
   cache some symbols related to the JIT interface; and for
   JIT-created objfiles, it holds some information about the
   jit_code_entry.  */

struct jit_objfile_data
{
  /* Symbol for __jit_debug_register_code.  */
  struct minimal_symbol *register_code;

  /* Symbol for __jit_debug_descriptor.  */
  struct minimal_symbol *descriptor;

  /* Address of struct jit_code_entry in this objfile.  This is only
     non-zero for objfiles that represent code created by the JIT.  */
  CORE_ADDR addr;
};

/* Fetch the jit_objfile_data associated with OBJF.  If no data exists
   yet, make a new structure and attach it.  */

static struct jit_objfile_data *
get_jit_objfile_data (struct objfile *objf)
{
  struct jit_objfile_data *objf_data;

  objf_data = (struct jit_objfile_data *) objfile_data (objf, jit_objfile_data);
  if (objf_data == NULL)
    {
      objf_data = XCNEW (struct jit_objfile_data);
      set_objfile_data (objf, jit_objfile_data, objf_data);
    }

  return objf_data;
}

/* Remember OBJFILE has been created for struct jit_code_entry located
   at inferior address ENTRY.  */

static void
add_objfile_entry (struct objfile *objfile, CORE_ADDR entry)
{
  struct jit_objfile_data *objf_data;

  objf_data = get_jit_objfile_data (objfile);
  objf_data->addr = entry;
}

/* Return jit_program_space_data for current program space.  Allocate
   if not already present.  */

static struct jit_program_space_data *
get_jit_program_space_data (void)
{
  struct jit_program_space_data *ps_data;

  ps_data
    = ((struct jit_program_space_data *)
       program_space_data (current_program_space, jit_program_space_data));
  if (ps_data == NULL)
    {
      ps_data = XCNEW (struct jit_program_space_data);
      set_program_space_data (current_program_space, jit_program_space_data,
			      ps_data);
    }

  return ps_data;
}

static void
jit_program_space_data_cleanup (struct program_space *ps, void *arg)
{
  xfree (arg);
}

/* Helper function for reading the global JIT descriptor from remote
   memory.  Returns 1 if all went well, 0 otherwise.  */

static int
jit_read_descriptor (struct gdbarch *gdbarch,
		     struct jit_descriptor *descriptor,
		     struct jit_program_space_data *ps_data)
{
  int err;
  struct type *ptr_type;
  int ptr_size;
  int desc_size;
  gdb_byte *desc_buf;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct jit_objfile_data *objf_data;

  if (ps_data->objfile == NULL)
    return 0;
  objf_data = get_jit_objfile_data (ps_data->objfile);
  if (objf_data->descriptor == NULL)
    return 0;

  if (jit_debug)
    fprintf_unfiltered (gdb_stdlog,
			"jit_read_descriptor, descriptor_addr = %s\n",
			paddress (gdbarch, MSYMBOL_VALUE_ADDRESS (ps_data->objfile,
								  objf_data->descriptor)));

  /* Figure out how big the descriptor is on the remote and how to read it.  */
  ptr_type = builtin_type (gdbarch)->builtin_data_ptr;
  ptr_size = TYPE_LENGTH (ptr_type);
  desc_size = 8 + 2 * ptr_size;  /* Two 32-bit ints and two pointers.  */
  desc_buf = (gdb_byte *) alloca (desc_size);

  /* Read the descriptor.  */
  err = target_read_memory (MSYMBOL_VALUE_ADDRESS (ps_data->objfile,
						   objf_data->descriptor),
			    desc_buf, desc_size);
  if (err)
    {
      printf_unfiltered (_("Unable to read JIT descriptor from "
			   "remote memory\n"));
      return 0;
    }

  /* Fix the endianness to match the host.  */
  descriptor->version = extract_unsigned_integer (&desc_buf[0], 4, byte_order);
  descriptor->action_flag =
      extract_unsigned_integer (&desc_buf[4], 4, byte_order);
  descriptor->relevant_entry = extract_typed_address (&desc_buf[8], ptr_type);
  descriptor->first_entry =
      extract_typed_address (&desc_buf[8 + ptr_size], ptr_type);

  return 1;
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

  /* Figure out where the longlong value will be.  */
  align_bytes = gdbarch_long_long_align_bit (gdbarch) / 8;
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
  /* gdb_blocks are linked into a tree structure.  Next points to the
     next node at the same depth as this block and parent to the
     parent gdb_block.  */
  struct gdb_block *next, *parent;

  /* Points to the "real" block that is being built out of this
     instance.  This block will be added to a blockvector, which will
     then be added to a symtab.  */
  struct block *real_block;

  /* The first and last code address corresponding to this block.  */
  CORE_ADDR begin, end;

  /* The name of this block (if any).  If this is non-NULL, the
     FUNCTION symbol symbol is set to this value.  */
  const char *name;
};

/* Proxy object for building a symtab.  */

struct gdb_symtab
{
  /* The list of blocks in this symtab.  These will eventually be
     converted to real blocks.  */
  struct gdb_block *blocks;

  /* The number of blocks inserted.  */
  int nblocks;

  /* A mapping between line numbers to PC.  */
  struct linetable *linetable;

  /* The source file for this symtab.  */
  const char *file_name;
  struct gdb_symtab *next;
};

/* Proxy object for building an object.  */

struct gdb_object
{
  struct gdb_symtab *symtabs;
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
  return XCNEW (struct gdb_object);
}

/* Readers call into this function to open a new gdb_symtab, which,
   again, is passed around to other callbacks.  */

static struct gdb_symtab *
jit_symtab_open_impl (struct gdb_symbol_callbacks *cb,
                      struct gdb_object *object,
                      const char *file_name)
{
  struct gdb_symtab *ret;

  /* CB stays unused.  See comment in jit_object_open_impl.  */

  ret = XCNEW (struct gdb_symtab);
  ret->file_name = file_name ? xstrdup (file_name) : xstrdup ("");
  ret->next = object->symtabs;
  object->symtabs = ret;
  return ret;
}

/* Returns true if the block corresponding to old should be placed
   before the block corresponding to new in the final blockvector.  */

static int
compare_block (const struct gdb_block *const old,
               const struct gdb_block *const newobj)
{
  if (old == NULL)
    return 1;
  if (old->begin < newobj->begin)
    return 1;
  else if (old->begin == newobj->begin)
    {
      if (old->end > newobj->end)
        return 1;
      else
        return 0;
    }
  else
    return 0;
}

/* Called by readers to open a new gdb_block.  This function also
   inserts the new gdb_block in the correct place in the corresponding
   gdb_symtab.  */

static struct gdb_block *
jit_block_open_impl (struct gdb_symbol_callbacks *cb,
                     struct gdb_symtab *symtab, struct gdb_block *parent,
                     GDB_CORE_ADDR begin, GDB_CORE_ADDR end, const char *name)
{
  struct gdb_block *block = XCNEW (struct gdb_block);

  block->next = symtab->blocks;
  block->begin = (CORE_ADDR) begin;
  block->end = (CORE_ADDR) end;
  block->name = name ? xstrdup (name) : NULL;
  block->parent = parent;

  /* Ensure that the blocks are inserted in the correct (reverse of
     the order expected by blockvector).  */
  if (compare_block (symtab->blocks, block))
    {
      symtab->blocks = block;
    }
  else
    {
      struct gdb_block *i = symtab->blocks;

      for (;; i = i->next)
        {
          /* Guaranteed to terminate, since compare_block (NULL, _)
             returns 1.  */
          if (compare_block (i->next, block))
            {
              block->next = i->next;
              i->next = block;
              break;
            }
        }
    }
  symtab->nblocks++;

  return block;
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
  stab->linetable = (struct linetable *) xmalloc (alloc_len);
  stab->linetable->nitems = nlines;
  for (i = 0; i < nlines; i++)
    {
      stab->linetable->item[i].pc = (CORE_ADDR) map[i].pc;
      stab->linetable->item[i].line = map[i].line;
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
  struct gdb_block *gdb_block_iter, *gdb_block_iter_tmp;
  struct block *block_iter;
  int actual_nblocks, i;
  size_t blockvector_size;
  CORE_ADDR begin, end;
  struct blockvector *bv;

  actual_nblocks = FIRST_LOCAL_BLOCK + stab->nblocks;

  cust = allocate_compunit_symtab (objfile, stab->file_name);
  allocate_symtab (cust, stab->file_name);
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
      memcpy (SYMTAB_LINETABLE (COMPUNIT_FILETABS (cust)), stab->linetable,
	      size);
    }

  blockvector_size = (sizeof (struct blockvector)
                      + (actual_nblocks - 1) * sizeof (struct block *));
  bv = (struct blockvector *) obstack_alloc (&objfile->objfile_obstack,
					     blockvector_size);
  COMPUNIT_BLOCKVECTOR (cust) = bv;

  /* (begin, end) will contain the PC range this entire blockvector
     spans.  */
  BLOCKVECTOR_MAP (bv) = NULL;
  begin = stab->blocks->begin;
  end = stab->blocks->end;
  BLOCKVECTOR_NBLOCKS (bv) = actual_nblocks;

  /* First run over all the gdb_block objects, creating a real block
     object for each.  Simultaneously, keep setting the real_block
     fields.  */
  for (i = (actual_nblocks - 1), gdb_block_iter = stab->blocks;
       i >= FIRST_LOCAL_BLOCK;
       i--, gdb_block_iter = gdb_block_iter->next)
    {
      struct block *new_block = allocate_block (&objfile->objfile_obstack);
      struct symbol *block_name = allocate_symbol (objfile);
      struct type *block_type = arch_type (get_objfile_arch (objfile),
					   TYPE_CODE_VOID,
					   1,
					   "void");

      BLOCK_DICT (new_block) = dict_create_linear (&objfile->objfile_obstack,
                                                   NULL);
      /* The address range.  */
      BLOCK_START (new_block) = (CORE_ADDR) gdb_block_iter->begin;
      BLOCK_END (new_block) = (CORE_ADDR) gdb_block_iter->end;

      /* The name.  */
      SYMBOL_DOMAIN (block_name) = VAR_DOMAIN;
      SYMBOL_ACLASS_INDEX (block_name) = LOC_BLOCK;
      symbol_set_symtab (block_name, COMPUNIT_FILETABS (cust));
      SYMBOL_TYPE (block_name) = lookup_function_type (block_type);
      SYMBOL_BLOCK_VALUE (block_name) = new_block;

      block_name->ginfo.name
	= (const char *) obstack_copy0 (&objfile->objfile_obstack,
					gdb_block_iter->name,
					strlen (gdb_block_iter->name));

      BLOCK_FUNCTION (new_block) = block_name;

      BLOCKVECTOR_BLOCK (bv, i) = new_block;
      if (begin > BLOCK_START (new_block))
        begin = BLOCK_START (new_block);
      if (end < BLOCK_END (new_block))
        end = BLOCK_END (new_block);

      gdb_block_iter->real_block = new_block;
    }

  /* Now add the special blocks.  */
  block_iter = NULL;
  for (i = 0; i < FIRST_LOCAL_BLOCK; i++)
    {
      struct block *new_block;

      new_block = (i == GLOBAL_BLOCK
		   ? allocate_global_block (&objfile->objfile_obstack)
		   : allocate_block (&objfile->objfile_obstack));
      BLOCK_DICT (new_block) = dict_create_linear (&objfile->objfile_obstack,
                                                   NULL);
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
  for (gdb_block_iter = stab->blocks;
       gdb_block_iter;
       gdb_block_iter = gdb_block_iter->next)
    {
      if (gdb_block_iter->parent != NULL)
	{
	  /* If the plugin specifically mentioned a parent block, we
	     use that.  */
	  BLOCK_SUPERBLOCK (gdb_block_iter->real_block) =
	    gdb_block_iter->parent->real_block;
	}
      else
	{
	  /* And if not, we set a default parent block.  */
	  BLOCK_SUPERBLOCK (gdb_block_iter->real_block) =
	    BLOCKVECTOR_BLOCK (bv, STATIC_BLOCK);
	}
    }

  /* Free memory.  */
  gdb_block_iter = stab->blocks;

  for (gdb_block_iter = stab->blocks, gdb_block_iter_tmp = gdb_block_iter->next;
       gdb_block_iter;
       gdb_block_iter = gdb_block_iter_tmp)
    {
      xfree ((void *) gdb_block_iter->name);
      xfree (gdb_block_iter);
    }
  xfree (stab->linetable);
  xfree ((char *) stab->file_name);
  xfree (stab);
}

/* Called when closing a gdb_objfile.  Converts OBJ to a proper
   objfile.  */

static void
jit_object_close_impl (struct gdb_symbol_callbacks *cb,
                       struct gdb_object *obj)
{
  struct gdb_symtab *i, *j;
  struct objfile *objfile;
  jit_dbg_reader_data *priv_data;

  priv_data = (jit_dbg_reader_data *) cb->priv_data;

  objfile = allocate_objfile (NULL, "<< JIT compiled code >>",
			      OBJF_NOT_FILENAME);
  objfile->per_bfd->gdbarch = target_gdbarch ();

  terminate_minimal_symbol_table (objfile);

  j = NULL;
  for (i = obj->symtabs; i; i = j)
    {
      j = i->next;
      finalize_symtab (i, objfile);
    }
  add_objfile_entry (objfile, *priv_data);
  xfree (obj);
}

/* Try to read CODE_ENTRY using the loaded jit reader (if any).
   ENTRY_ADDR is the address of the struct jit_code_entry in the
   inferior address space.  */

static int
jit_reader_try_read_symtab (struct jit_code_entry *code_entry,
                            CORE_ADDR entry_addr)
{
  gdb_byte *gdb_mem;
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

  gdb_mem = (gdb_byte *) xmalloc (code_entry->symfile_size);

  status = 1;
  TRY
    {
      if (target_read_memory (code_entry->symfile_addr, gdb_mem,
			      code_entry->symfile_size))
	status = 0;
    }
  CATCH (e, RETURN_MASK_ALL)
    {
      status = 0;
    }
  END_CATCH

  if (status)
    {
      funcs = loaded_jit_reader->functions;
      if (funcs->read (funcs, &callbacks, gdb_mem, code_entry->symfile_size)
          != GDB_SUCCESS)
        status = 0;
    }

  xfree (gdb_mem);
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
  bfd *nbfd;
  struct section_addr_info *sai;
  struct bfd_section *sec;
  struct objfile *objfile;
  struct cleanup *old_cleanups;
  int i;
  const struct bfd_arch_info *b;

  if (jit_debug)
    fprintf_unfiltered (gdb_stdlog,
			"jit_register_code, symfile_addr = %s, "
			"symfile_size = %s\n",
			paddress (gdbarch, code_entry->symfile_addr),
			pulongest (code_entry->symfile_size));

  nbfd = bfd_open_from_target_memory (code_entry->symfile_addr,
                                      code_entry->symfile_size, gnutarget);
  if (nbfd == NULL)
    {
      puts_unfiltered (_("Error opening JITed symbol file, ignoring it.\n"));
      return;
    }

  /* Check the format.  NOTE: This initializes important data that GDB uses!
     We would segfault later without this line.  */
  if (!bfd_check_format (nbfd, bfd_object))
    {
      printf_unfiltered (_("\
JITed symbol file is not an object file, ignoring it.\n"));
      gdb_bfd_unref (nbfd);
      return;
    }

  /* Check bfd arch.  */
  b = gdbarch_bfd_arch_info (gdbarch);
  if (b->compatible (b, bfd_get_arch_info (nbfd)) != b)
    warning (_("JITed object file architecture %s is not compatible "
               "with target architecture %s."), bfd_get_arch_info
             (nbfd)->printable_name, b->printable_name);

  /* Read the section address information out of the symbol file.  Since the
     file is generated by the JIT at runtime, it should all of the absolute
     addresses that we care about.  */
  sai = alloc_section_addr_info (bfd_count_sections (nbfd));
  old_cleanups = make_cleanup_free_section_addr_info (sai);
  i = 0;
  for (sec = nbfd->sections; sec != NULL; sec = sec->next)
    if ((bfd_get_section_flags (nbfd, sec) & (SEC_ALLOC|SEC_LOAD)) != 0)
      {
        /* We assume that these virtual addresses are absolute, and do not
           treat them as offsets.  */
        sai->other[i].addr = bfd_get_section_vma (nbfd, sec);
        sai->other[i].name = xstrdup (bfd_get_section_name (nbfd, sec));
        sai->other[i].sectindex = sec->index;
        ++i;
      }
  sai->num_sections = i;

  /* This call does not take ownership of SAI.  */
  make_cleanup_bfd_unref (nbfd);
  objfile = symbol_file_add_from_bfd (nbfd, bfd_get_filename (nbfd), 0, sai,
				      OBJF_SHARED | OBJF_NOT_FILENAME, NULL);

  do_cleanups (old_cleanups);
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

/* This function unregisters JITed code and frees the corresponding
   objfile.  */

static void
jit_unregister_code (struct objfile *objfile)
{
  free_objfile (objfile);
}

/* Look up the objfile with this code entry address.  */

static struct objfile *
jit_find_objf_with_entry_addr (CORE_ADDR entry_addr)
{
  struct objfile *objf;

  ALL_OBJFILES (objf)
    {
      struct jit_objfile_data *objf_data;

      objf_data
	= (struct jit_objfile_data *) objfile_data (objf, jit_objfile_data);
      if (objf_data != NULL && objf_data->addr == entry_addr)
        return objf;
    }
  return NULL;
}

/* This is called when a breakpoint is deleted.  It updates the
   inferior's cache, if needed.  */

static void
jit_breakpoint_deleted (struct breakpoint *b)
{
  struct bp_location *iter;

  if (b->type != bp_jit_event)
    return;

  for (iter = b->loc; iter != NULL; iter = iter->next)
    {
      struct jit_program_space_data *ps_data;

      ps_data = ((struct jit_program_space_data *)
		 program_space_data (iter->pspace, jit_program_space_data));
      if (ps_data != NULL && ps_data->jit_breakpoint == iter->owner)
	{
	  ps_data->cached_code_address = 0;
	  ps_data->jit_breakpoint = NULL;
	}
    }
}

/* (Re-)Initialize the jit breakpoint if necessary.
   Return 0 if the jit breakpoint has been successfully initialized.  */

static int
jit_breakpoint_re_set_internal (struct gdbarch *gdbarch,
				struct jit_program_space_data *ps_data)
{
  struct bound_minimal_symbol reg_symbol;
  struct bound_minimal_symbol desc_symbol;
  struct jit_objfile_data *objf_data;
  CORE_ADDR addr;

  if (ps_data->objfile == NULL)
    {
      /* Lookup the registration symbol.  If it is missing, then we
	 assume we are not attached to a JIT.  */
      reg_symbol = lookup_minimal_symbol_and_objfile (jit_break_name);
      if (reg_symbol.minsym == NULL
	  || BMSYMBOL_VALUE_ADDRESS (reg_symbol) == 0)
	return 1;

      desc_symbol = lookup_minimal_symbol (jit_descriptor_name, NULL,
					   reg_symbol.objfile);
      if (desc_symbol.minsym == NULL
	  || BMSYMBOL_VALUE_ADDRESS (desc_symbol) == 0)
	return 1;

      objf_data = get_jit_objfile_data (reg_symbol.objfile);
      objf_data->register_code = reg_symbol.minsym;
      objf_data->descriptor = desc_symbol.minsym;

      ps_data->objfile = reg_symbol.objfile;
    }
  else
    objf_data = get_jit_objfile_data (ps_data->objfile);

  addr = MSYMBOL_VALUE_ADDRESS (ps_data->objfile, objf_data->register_code);

  if (jit_debug)
    fprintf_unfiltered (gdb_stdlog,
			"jit_breakpoint_re_set_internal, "
			"breakpoint_addr = %s\n",
			paddress (gdbarch, addr));

  if (ps_data->cached_code_address == addr)
    return 0;

  /* Delete the old breakpoint.  */
  if (ps_data->jit_breakpoint != NULL)
    delete_breakpoint (ps_data->jit_breakpoint);

  /* Put a breakpoint in the registration symbol.  */
  ps_data->cached_code_address = addr;
  ps_data->jit_breakpoint = create_jit_event_breakpoint (gdbarch, addr);

  return 0;
}

/* The private data passed around in the frame unwind callback
   functions.  */

struct jit_unwind_private
{
  /* Cached register values.  See jit_frame_sniffer to see how this
     works.  */
  struct regcache *regcache;

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

  regcache_raw_set_cached_value (priv->regcache, gdb_reg, value->value);
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
  regcache_xfree (priv_data->regcache);
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
  struct address_space *aspace;
  struct gdbarch *gdbarch;

  callbacks.reg_get = jit_unwind_reg_get_impl;
  callbacks.reg_set = jit_unwind_reg_set_impl;
  callbacks.target_read = jit_target_read_impl;

  if (loaded_jit_reader == NULL)
    return 0;

  funcs = loaded_jit_reader->functions;

  gdb_assert (!*cache);

  aspace = get_frame_address_space (this_frame);
  gdbarch = get_frame_arch (this_frame);

  *cache = XCNEW (struct jit_unwind_private);
  priv_data = (struct jit_unwind_private *) *cache;
  priv_data->regcache = regcache_xmalloc (gdbarch, aspace);
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

  gdbarch = get_regcache_arch (priv->regcache);
  if (reg < gdbarch_num_regs (gdbarch))
    {
      gdb_byte *buf = (gdb_byte *) alloca (register_size (gdbarch, reg));
      enum register_status status;

      status = regcache_raw_read (priv->regcache, reg, buf);
      if (status == REG_VALID)
	return frame_unwind_got_bytes (this_frame, reg, buf);
      else
	return frame_unwind_got_optimized (this_frame, reg);
    }
  else
    return gdbarch_pseudo_register_read_value (gdbarch, priv->regcache, reg);
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
  struct jit_program_space_data *ps_data;
  CORE_ADDR cur_entry_addr;

  if (jit_debug)
    fprintf_unfiltered (gdb_stdlog, "jit_inferior_init\n");

  jit_prepend_unwinder (gdbarch);

  ps_data = get_jit_program_space_data ();
  if (jit_breakpoint_re_set_internal (gdbarch, ps_data) != 0)
    return;

  /* Read the descriptor so we can check the version number and load
     any already JITed functions.  */
  if (!jit_read_descriptor (gdbarch, &descriptor, ps_data))
    return;

  /* Check that the version number agrees with that we support.  */
  if (descriptor.version != 1)
    {
      printf_unfiltered (_("Unsupported JIT protocol version %ld "
			   "in descriptor (expected 1)\n"),
			 (long) descriptor.version);
      return;
    }

  /* If we've attached to a running program, we need to check the descriptor
     to register any functions that were already generated.  */
  for (cur_entry_addr = descriptor.first_entry;
       cur_entry_addr != 0;
       cur_entry_addr = cur_entry.next_entry)
    {
      jit_read_code_entry (gdbarch, cur_entry_addr, &cur_entry);

      /* This hook may be called many times during setup, so make sure we don't
         add the same symbol file twice.  */
      if (jit_find_objf_with_entry_addr (cur_entry_addr) != NULL)
        continue;

      jit_register_code (gdbarch, cur_entry_addr, &cur_entry);
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
  jit_breakpoint_re_set_internal (target_gdbarch (),
				  get_jit_program_space_data ());
}

/* This function cleans up any code entries left over when the
   inferior exits.  We get left over code when the inferior exits
   without unregistering its code, for example when it crashes.  */

static void
jit_inferior_exit_hook (struct inferior *inf)
{
  struct objfile *objf;
  struct objfile *temp;

  ALL_OBJFILES_SAFE (objf, temp)
    {
      struct jit_objfile_data *objf_data
	= (struct jit_objfile_data *) objfile_data (objf, jit_objfile_data);

      if (objf_data != NULL && objf_data->addr != 0)
	jit_unregister_code (objf);
    }
}

void
jit_event_handler (struct gdbarch *gdbarch)
{
  struct jit_descriptor descriptor;
  struct jit_code_entry code_entry;
  CORE_ADDR entry_addr;
  struct objfile *objf;

  /* Read the descriptor from remote memory.  */
  if (!jit_read_descriptor (gdbarch, &descriptor,
			    get_jit_program_space_data ()))
    return;
  entry_addr = descriptor.relevant_entry;

  /* Do the corresponding action.  */
  switch (descriptor.action_flag)
    {
    case JIT_NOACTION:
      break;
    case JIT_REGISTER:
      jit_read_code_entry (gdbarch, entry_addr, &code_entry);
      jit_register_code (gdbarch, entry_addr, &code_entry);
      break;
    case JIT_UNREGISTER:
      objf = jit_find_objf_with_entry_addr (entry_addr);
      if (objf == NULL)
	printf_unfiltered (_("Unable to find JITed code "
			     "entry at address: %s\n"),
			   paddress (gdbarch, entry_addr));
      else
        jit_unregister_code (objf);

      break;
    default:
      error (_("Unknown action_flag value in JIT descriptor!"));
      break;
    }
}

/* Called to free the data allocated to the jit_program_space_data slot.  */

static void
free_objfile_data (struct objfile *objfile, void *data)
{
  struct jit_objfile_data *objf_data = (struct jit_objfile_data *) data;

  if (objf_data->register_code != NULL)
    {
      struct jit_program_space_data *ps_data;

      ps_data
	= ((struct jit_program_space_data *)
	   program_space_data (objfile->pspace, jit_program_space_data));
      if (ps_data != NULL && ps_data->objfile == objfile)
	{
	  ps_data->objfile = NULL;
	  delete_breakpoint (ps_data->jit_breakpoint);
	  ps_data->cached_code_address = 0;
	}
    }

  xfree (data);
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

/* Provide a prototype to silence -Wmissing-prototypes.  */

extern void _initialize_jit (void);

void
_initialize_jit (void)
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

  observer_attach_inferior_created (jit_inferior_created);
  observer_attach_inferior_exit (jit_inferior_exit_hook);
  observer_attach_breakpoint_deleted (jit_breakpoint_deleted);

  jit_objfile_data =
    register_objfile_data_with_cleanup (NULL, free_objfile_data);
  jit_program_space_data =
    register_program_space_data_with_cleanup (NULL,
					      jit_program_space_data_cleanup);
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
