/* Load module for 'compile' command.

   Copyright (C) 2014-2016 Free Software Foundation, Inc.

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
#include "compile-object-load.h"
#include "compile-internal.h"
#include "command.h"
#include "objfiles.h"
#include "gdbcore.h"
#include "readline/tilde.h"
#include "bfdlink.h"
#include "gdbcmd.h"
#include "regcache.h"
#include "inferior.h"
#include "compile.h"
#include "block.h"
#include "arch-utils.h"

/* Track inferior memory reserved by inferior mmap.  */

struct munmap_list
{
  struct munmap_list *next;
  CORE_ADDR addr, size;
};

/* Add inferior mmap memory range ADDR..ADDR+SIZE (exclusive) to list
   HEADP.  *HEADP needs to be initialized to NULL.  */

static void
munmap_list_add (struct munmap_list **headp, CORE_ADDR addr, CORE_ADDR size)
{
  struct munmap_list *head_new = XNEW (struct munmap_list);

  head_new->next = *headp;
  *headp = head_new;
  head_new->addr = addr;
  head_new->size = size;
}

/* Free list of inferior mmap memory ranges HEAD.  HEAD is the first
   element of the list, it can be NULL.  After calling this function
   HEAD pointer is invalid and the possible list needs to be
   reinitialized by caller to NULL.  */

void
munmap_list_free (struct munmap_list *head)
{
  while (head)
    {
      struct munmap_list *todo = head;

      head = todo->next;
      gdbarch_infcall_munmap (target_gdbarch (), todo->addr, todo->size);
      xfree (todo);
    }
}

/* Stub for munmap_list_free suitable for make_cleanup.  Contrary to
   munmap_list_free this function's parameter is a pointer to the first
   list element pointer.  */

static void
munmap_listp_free_cleanup (void *headp_voidp)
{
  struct munmap_list **headp = (struct munmap_list **) headp_voidp;

  munmap_list_free (*headp);
}

/* Helper data for setup_sections.  */

struct setup_sections_data
{
  /* Size of all recent sections with matching LAST_PROT.  */
  CORE_ADDR last_size;

  /* First section matching LAST_PROT.  */
  asection *last_section_first;

  /* Memory protection like the prot parameter of gdbarch_infcall_mmap. */
  unsigned last_prot;

  /* Maximum of alignments of all sections matching LAST_PROT.
     This value is always at least 1.  This value is always a power of 2.  */
  CORE_ADDR last_max_alignment;

  /* List of inferior mmap ranges where setup_sections should add its
     next range.  */
  struct munmap_list **munmap_list_headp;
};

/* Place all ABFD sections next to each other obeying all constraints.  */

static void
setup_sections (bfd *abfd, asection *sect, void *data_voidp)
{
  struct setup_sections_data *data = (struct setup_sections_data *) data_voidp;
  CORE_ADDR alignment;
  unsigned prot;

  if (sect != NULL)
    {
      /* It is required by later bfd_get_relocated_section_contents.  */
      if (sect->output_section == NULL)
	sect->output_section = sect;

      if ((bfd_get_section_flags (abfd, sect) & SEC_ALLOC) == 0)
	return;

      /* Make the memory always readable.  */
      prot = GDB_MMAP_PROT_READ;
      if ((bfd_get_section_flags (abfd, sect) & SEC_READONLY) == 0)
	prot |= GDB_MMAP_PROT_WRITE;
      if ((bfd_get_section_flags (abfd, sect) & SEC_CODE) != 0)
	prot |= GDB_MMAP_PROT_EXEC;

      if (compile_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "module \"%s\" section \"%s\" size %s prot %u\n",
			    bfd_get_filename (abfd),
			    bfd_get_section_name (abfd, sect),
			    paddress (target_gdbarch (),
				      bfd_get_section_size (sect)),
			    prot);
    }
  else
    prot = -1;

  if (sect == NULL
      || (data->last_prot != prot && bfd_get_section_size (sect) != 0))
    {
      CORE_ADDR addr;
      asection *sect_iter;

      if (data->last_size != 0)
	{
	  addr = gdbarch_infcall_mmap (target_gdbarch (), data->last_size,
				       data->last_prot);
	  munmap_list_add (data->munmap_list_headp, addr, data->last_size);
	  if (compile_debug)
	    fprintf_unfiltered (gdb_stdlog,
				"allocated %s bytes at %s prot %u\n",
				paddress (target_gdbarch (), data->last_size),
				paddress (target_gdbarch (), addr),
				data->last_prot);
	}
      else
	addr = 0;

      if ((addr & (data->last_max_alignment - 1)) != 0)
	error (_("Inferior compiled module address %s "
		 "is not aligned to BFD required %s."),
	       paddress (target_gdbarch (), addr),
	       paddress (target_gdbarch (), data->last_max_alignment));

      for (sect_iter = data->last_section_first; sect_iter != sect;
	   sect_iter = sect_iter->next)
	if ((bfd_get_section_flags (abfd, sect_iter) & SEC_ALLOC) != 0)
	  bfd_set_section_vma (abfd, sect_iter,
			       addr + bfd_get_section_vma (abfd, sect_iter));

      data->last_size = 0;
      data->last_section_first = sect;
      data->last_prot = prot;
      data->last_max_alignment = 1;
    }

  if (sect == NULL)
    return;

  alignment = ((CORE_ADDR) 1) << bfd_get_section_alignment (abfd, sect);
  data->last_max_alignment = max (data->last_max_alignment, alignment);

  data->last_size = (data->last_size + alignment - 1) & -alignment;

  bfd_set_section_vma (abfd, sect, data->last_size);

  data->last_size += bfd_get_section_size (sect);
  data->last_size = (data->last_size + alignment - 1) & -alignment;
}

/* Helper for link_callbacks callbacks vector.  */

static void
link_callbacks_multiple_definition (struct bfd_link_info *link_info,
				    struct bfd_link_hash_entry *h, bfd *nbfd,
				    asection *nsec, bfd_vma nval)
{
  bfd *abfd = link_info->input_bfds;

  if (link_info->allow_multiple_definition)
    return;
  warning (_("Compiled module \"%s\": multiple symbol definitions: %s"),
	   bfd_get_filename (abfd), h->root.string);
}

/* Helper for link_callbacks callbacks vector.  */

static void
link_callbacks_warning (struct bfd_link_info *link_info, const char *xwarning,
                        const char *symbol, bfd *abfd, asection *section,
			bfd_vma address)
{
  warning (_("Compiled module \"%s\" section \"%s\": warning: %s"),
	   bfd_get_filename (abfd), bfd_get_section_name (abfd, section),
	   xwarning);
}

/* Helper for link_callbacks callbacks vector.  */

static void
link_callbacks_undefined_symbol (struct bfd_link_info *link_info,
				 const char *name, bfd *abfd, asection *section,
				 bfd_vma address, bfd_boolean is_fatal)
{
  warning (_("Cannot resolve relocation to \"%s\" "
	     "from compiled module \"%s\" section \"%s\"."),
	   name, bfd_get_filename (abfd), bfd_get_section_name (abfd, section));
}

/* Helper for link_callbacks callbacks vector.  */

static void
link_callbacks_reloc_overflow (struct bfd_link_info *link_info,
			       struct bfd_link_hash_entry *entry,
			       const char *name, const char *reloc_name,
			       bfd_vma addend, bfd *abfd, asection *section,
			       bfd_vma address)
{
}

/* Helper for link_callbacks callbacks vector.  */

static void
link_callbacks_reloc_dangerous (struct bfd_link_info *link_info,
				const char *message, bfd *abfd,
				asection *section, bfd_vma address)
{
  warning (_("Compiled module \"%s\" section \"%s\": dangerous "
	     "relocation: %s\n"),
	   bfd_get_filename (abfd), bfd_get_section_name (abfd, section),
	   message);
}

/* Helper for link_callbacks callbacks vector.  */

static void
link_callbacks_unattached_reloc (struct bfd_link_info *link_info,
				 const char *name, bfd *abfd, asection *section,
				 bfd_vma address)
{
  warning (_("Compiled module \"%s\" section \"%s\": unattached "
	     "relocation: %s\n"),
	   bfd_get_filename (abfd), bfd_get_section_name (abfd, section),
	   name);
}

/* Helper for link_callbacks callbacks vector.  */

static void link_callbacks_einfo (const char *fmt, ...)
  ATTRIBUTE_PRINTF (1, 2);

static void
link_callbacks_einfo (const char *fmt, ...)
{
  struct cleanup *cleanups;
  va_list ap;
  char *str;

  va_start (ap, fmt);
  str = xstrvprintf (fmt, ap);
  va_end (ap);
  cleanups = make_cleanup (xfree, str);

  warning (_("Compile module: warning: %s"), str);

  do_cleanups (cleanups);
}

/* Helper for bfd_get_relocated_section_contents.
   Only these symbols are set by bfd_simple_get_relocated_section_contents
   but bfd/ seems to use even the NULL ones without checking them first.  */

static const struct bfd_link_callbacks link_callbacks =
{
  NULL, /* add_archive_element */
  link_callbacks_multiple_definition, /* multiple_definition */
  NULL, /* multiple_common */
  NULL, /* add_to_set */
  NULL, /* constructor */
  link_callbacks_warning, /* warning */
  link_callbacks_undefined_symbol, /* undefined_symbol */
  link_callbacks_reloc_overflow, /* reloc_overflow */
  link_callbacks_reloc_dangerous, /* reloc_dangerous */
  link_callbacks_unattached_reloc, /* unattached_reloc */
  NULL, /* notice */
  link_callbacks_einfo, /* einfo */
  NULL, /* info */
  NULL, /* minfo */
  NULL, /* override_segment_assignment */
};

struct link_hash_table_cleanup_data
{
  bfd *abfd;
  bfd *link_next;
};

/* Cleanup callback for struct bfd_link_info.  */

static void
link_hash_table_free (void *d)
{
  struct link_hash_table_cleanup_data *data
    = (struct link_hash_table_cleanup_data *) d;

  if (data->abfd->is_linker_output)
    (*data->abfd->link.hash->hash_table_free) (data->abfd);
  data->abfd->link.next = data->link_next;
}

/* Relocate and store into inferior memory each section SECT of ABFD.  */

static void
copy_sections (bfd *abfd, asection *sect, void *data)
{
  asymbol **symbol_table = (asymbol **) data;
  bfd_byte *sect_data, *sect_data_got;
  struct cleanup *cleanups;
  struct bfd_link_info link_info;
  struct bfd_link_order link_order;
  CORE_ADDR inferior_addr;
  struct link_hash_table_cleanup_data cleanup_data;

  if ((bfd_get_section_flags (abfd, sect) & (SEC_ALLOC | SEC_LOAD))
      != (SEC_ALLOC | SEC_LOAD))
    return;

  if (bfd_get_section_size (sect) == 0)
    return;

  /* Mostly a copy of bfd_simple_get_relocated_section_contents which GDB
     cannot use as it does not report relocations to undefined symbols.  */
  memset (&link_info, 0, sizeof (link_info));
  link_info.output_bfd = abfd;
  link_info.input_bfds = abfd;
  link_info.input_bfds_tail = &abfd->link.next;

  cleanup_data.abfd = abfd;
  cleanup_data.link_next = abfd->link.next;

  abfd->link.next = NULL;
  link_info.hash = bfd_link_hash_table_create (abfd);

  cleanups = make_cleanup (link_hash_table_free, &cleanup_data);
  link_info.callbacks = &link_callbacks;

  memset (&link_order, 0, sizeof (link_order));
  link_order.next = NULL;
  link_order.type = bfd_indirect_link_order;
  link_order.offset = 0;
  link_order.size = bfd_get_section_size (sect);
  link_order.u.indirect.section = sect;

  sect_data = (bfd_byte *) xmalloc (bfd_get_section_size (sect));
  make_cleanup (xfree, sect_data);

  sect_data_got = bfd_get_relocated_section_contents (abfd, &link_info,
						      &link_order, sect_data,
						      FALSE, symbol_table);

  if (sect_data_got == NULL)
    error (_("Cannot map compiled module \"%s\" section \"%s\": %s"),
	   bfd_get_filename (abfd), bfd_get_section_name (abfd, sect),
	   bfd_errmsg (bfd_get_error ()));
  gdb_assert (sect_data_got == sect_data);

  inferior_addr = bfd_get_section_vma (abfd, sect);
  if (0 != target_write_memory (inferior_addr, sect_data,
				bfd_get_section_size (sect)))
    error (_("Cannot write compiled module \"%s\" section \"%s\" "
	     "to inferior memory range %s-%s."),
	   bfd_get_filename (abfd), bfd_get_section_name (abfd, sect),
	   paddress (target_gdbarch (), inferior_addr),
	   paddress (target_gdbarch (),
		     inferior_addr + bfd_get_section_size (sect)));

  do_cleanups (cleanups);
}

/* Fetch the type of COMPILE_I_EXPR_PTR_TYPE and COMPILE_I_EXPR_VAL
   symbols in OBJFILE so we can calculate how much memory to allocate
   for the out parameter.  This avoids needing a malloc in the generated
   code.  Throw an error if anything fails.
   GDB first tries to compile the code with COMPILE_I_PRINT_ADDRESS_SCOPE.
   If it finds user tries to print an array type this function returns
   NULL.  Caller will then regenerate the code with
   COMPILE_I_PRINT_VALUE_SCOPE, recompiles it again and finally runs it.
   This is because __auto_type array-to-pointer type conversion of
   COMPILE_I_EXPR_VAL which gets detected by COMPILE_I_EXPR_PTR_TYPE
   preserving the array type.  */

static struct type *
get_out_value_type (struct symbol *func_sym, struct objfile *objfile,
		    enum compile_i_scope_types scope)
{
  struct symbol *gdb_ptr_type_sym;
  /* Initialize it just to avoid a GCC false warning.  */
  struct symbol *gdb_val_sym = NULL;
  struct type *gdb_ptr_type, *gdb_type_from_ptr, *gdb_type, *retval;
  /* Initialize it just to avoid a GCC false warning.  */
  const struct block *block = NULL;
  const struct blockvector *bv;
  int nblocks = 0;
  int block_loop = 0;

  bv = SYMTAB_BLOCKVECTOR (func_sym->owner.symtab);
  nblocks = BLOCKVECTOR_NBLOCKS (bv);

  gdb_ptr_type_sym = NULL;
  for (block_loop = 0; block_loop < nblocks; block_loop++)
    {
      struct symbol *function = NULL;
      const struct block *function_block;

      block = BLOCKVECTOR_BLOCK (bv, block_loop);
      if (BLOCK_FUNCTION (block) != NULL)
	continue;
      gdb_val_sym = block_lookup_symbol (block, COMPILE_I_EXPR_VAL, VAR_DOMAIN);
      if (gdb_val_sym == NULL)
	continue;

      function_block = block;
      while (function_block != BLOCKVECTOR_BLOCK (bv, STATIC_BLOCK)
	     && function_block != BLOCKVECTOR_BLOCK (bv, GLOBAL_BLOCK))
	{
	  function_block = BLOCK_SUPERBLOCK (function_block);
	  function = BLOCK_FUNCTION (function_block);
	  if (function != NULL)
	    break;
	}
      if (function != NULL
	  && (BLOCK_SUPERBLOCK (function_block)
	      == BLOCKVECTOR_BLOCK (bv, STATIC_BLOCK))
	  && (strcmp (SYMBOL_LINKAGE_NAME (function), GCC_FE_WRAPPER_FUNCTION)
	      == 0))
	break;
    }
  if (block_loop == nblocks)
    error (_("No \"%s\" symbol found"), COMPILE_I_EXPR_PTR_TYPE);

  gdb_type = SYMBOL_TYPE (gdb_val_sym);
  gdb_type = check_typedef (gdb_type);

  gdb_ptr_type_sym = block_lookup_symbol (block, COMPILE_I_EXPR_PTR_TYPE,
					  VAR_DOMAIN);
  if (gdb_ptr_type_sym == NULL)
    error (_("No \"%s\" symbol found"), COMPILE_I_EXPR_PTR_TYPE);
  gdb_ptr_type = SYMBOL_TYPE (gdb_ptr_type_sym);
  gdb_ptr_type = check_typedef (gdb_ptr_type);
  if (TYPE_CODE (gdb_ptr_type) != TYPE_CODE_PTR)
    error (_("Type of \"%s\" is not a pointer"), COMPILE_I_EXPR_PTR_TYPE);
  gdb_type_from_ptr = TYPE_TARGET_TYPE (gdb_ptr_type);

  if (types_deeply_equal (gdb_type, gdb_type_from_ptr))
    {
      if (scope != COMPILE_I_PRINT_ADDRESS_SCOPE)
	error (_("Expected address scope in compiled module \"%s\"."),
	       objfile_name (objfile));
      return gdb_type;
    }

  if (TYPE_CODE (gdb_type) != TYPE_CODE_PTR)
    error (_("Invalid type code %d of symbol \"%s\" "
	     "in compiled module \"%s\"."),
	   TYPE_CODE (gdb_type_from_ptr), COMPILE_I_EXPR_VAL,
	   objfile_name (objfile));
  
  retval = gdb_type_from_ptr;
  switch (TYPE_CODE (gdb_type_from_ptr))
    {
    case TYPE_CODE_ARRAY:
      gdb_type_from_ptr = TYPE_TARGET_TYPE (gdb_type_from_ptr);
      break;
    case TYPE_CODE_FUNC:
      break;
    default:
      error (_("Invalid type code %d of symbol \"%s\" "
	       "in compiled module \"%s\"."),
	     TYPE_CODE (gdb_type_from_ptr), COMPILE_I_EXPR_PTR_TYPE,
	     objfile_name (objfile));
    }
  if (!types_deeply_equal (gdb_type_from_ptr,
			   TYPE_TARGET_TYPE (gdb_type)))
    error (_("Referenced types do not match for symbols \"%s\" and \"%s\" "
	     "in compiled module \"%s\"."),
	   COMPILE_I_EXPR_PTR_TYPE, COMPILE_I_EXPR_VAL,
	   objfile_name (objfile));
  if (scope == COMPILE_I_PRINT_ADDRESS_SCOPE)
    return NULL;
  return retval;
}

/* Fetch the type of first parameter of FUNC_SYM.
   Return NULL if FUNC_SYM has no parameters.  Throw an error otherwise.  */

static struct type *
get_regs_type (struct symbol *func_sym, struct objfile *objfile)
{
  struct type *func_type = SYMBOL_TYPE (func_sym);
  struct type *regsp_type, *regs_type;

  /* No register parameter present.  */
  if (TYPE_NFIELDS (func_type) == 0)
    return NULL;

  regsp_type = check_typedef (TYPE_FIELD_TYPE (func_type, 0));
  if (TYPE_CODE (regsp_type) != TYPE_CODE_PTR)
    error (_("Invalid type code %d of first parameter of function \"%s\" "
	     "in compiled module \"%s\"."),
	   TYPE_CODE (regsp_type), GCC_FE_WRAPPER_FUNCTION,
	   objfile_name (objfile));

  regs_type = check_typedef (TYPE_TARGET_TYPE (regsp_type));
  if (TYPE_CODE (regs_type) != TYPE_CODE_STRUCT)
    error (_("Invalid type code %d of dereferenced first parameter "
	     "of function \"%s\" in compiled module \"%s\"."),
	   TYPE_CODE (regs_type), GCC_FE_WRAPPER_FUNCTION,
	   objfile_name (objfile));

  return regs_type;
}

/* Store all inferior registers required by REGS_TYPE to inferior memory
   starting at inferior address REGS_BASE.  */

static void
store_regs (struct type *regs_type, CORE_ADDR regs_base)
{
  struct gdbarch *gdbarch = target_gdbarch ();
  struct regcache *regcache = get_thread_regcache (inferior_ptid);
  int fieldno;

  for (fieldno = 0; fieldno < TYPE_NFIELDS (regs_type); fieldno++)
    {
      const char *reg_name = TYPE_FIELD_NAME (regs_type, fieldno);
      ULONGEST reg_bitpos = TYPE_FIELD_BITPOS (regs_type, fieldno);
      ULONGEST reg_bitsize = TYPE_FIELD_BITSIZE (regs_type, fieldno);
      ULONGEST reg_offset;
      struct type *reg_type = check_typedef (TYPE_FIELD_TYPE (regs_type,
							      fieldno));
      ULONGEST reg_size = TYPE_LENGTH (reg_type);
      int regnum;
      struct value *regval;
      CORE_ADDR inferior_addr;

      if (strcmp (reg_name, COMPILE_I_SIMPLE_REGISTER_DUMMY) == 0)
	continue;

      if ((reg_bitpos % 8) != 0 || reg_bitsize != 0)
	error (_("Invalid register \"%s\" position %s bits or size %s bits"),
	       reg_name, pulongest (reg_bitpos), pulongest (reg_bitsize));
      reg_offset = reg_bitpos / 8;

      if (TYPE_CODE (reg_type) != TYPE_CODE_INT
	  && TYPE_CODE (reg_type) != TYPE_CODE_PTR)
	error (_("Invalid register \"%s\" type code %d"), reg_name,
	       TYPE_CODE (reg_type));

      regnum = compile_register_name_demangle (gdbarch, reg_name);

      regval = value_from_register (reg_type, regnum, get_current_frame ());
      if (value_optimized_out (regval))
	error (_("Register \"%s\" is optimized out."), reg_name);
      if (!value_entirely_available (regval))
	error (_("Register \"%s\" is not available."), reg_name);

      inferior_addr = regs_base + reg_offset;
      if (0 != target_write_memory (inferior_addr, value_contents (regval),
				    reg_size))
	error (_("Cannot write register \"%s\" to inferior memory at %s."),
	       reg_name, paddress (gdbarch, inferior_addr));
    }
}

/* Load OBJECT_FILE into inferior memory.  Throw an error otherwise.
   Caller must fully dispose the return value by calling compile_object_run.
   SOURCE_FILE's copy is stored into the returned object.
   Caller should free both OBJECT_FILE and SOURCE_FILE immediatelly after this
   function returns.
   Function returns NULL only for COMPILE_I_PRINT_ADDRESS_SCOPE when
   COMPILE_I_PRINT_VALUE_SCOPE should have been used instead.  */

struct compile_module *
compile_object_load (const char *object_file, const char *source_file,
		     enum compile_i_scope_types scope, void *scope_data)
{
  struct cleanup *cleanups, *cleanups_free_objfile;
  bfd *abfd;
  struct setup_sections_data setup_sections_data;
  CORE_ADDR addr, regs_addr, out_value_addr = 0;
  struct symbol *func_sym;
  struct type *func_type;
  struct bound_minimal_symbol bmsym;
  long storage_needed;
  asymbol **symbol_table, **symp;
  long number_of_symbols, missing_symbols;
  struct type *dptr_type = builtin_type (target_gdbarch ())->builtin_data_ptr;
  unsigned dptr_type_len = TYPE_LENGTH (dptr_type);
  struct compile_module *retval;
  struct type *regs_type, *out_value_type = NULL;
  char *filename, **matching;
  struct objfile *objfile;
  int expect_parameters;
  struct type *expect_return_type;
  struct munmap_list *munmap_list_head = NULL;

  filename = tilde_expand (object_file);
  cleanups = make_cleanup (xfree, filename);

  abfd = gdb_bfd_open (filename, gnutarget, -1);
  if (abfd == NULL)
    error (_("\"%s\": could not open as compiled module: %s"),
          filename, bfd_errmsg (bfd_get_error ()));
  make_cleanup_bfd_unref (abfd);

  if (!bfd_check_format_matches (abfd, bfd_object, &matching))
    error (_("\"%s\": not in loadable format: %s"),
          filename, gdb_bfd_errmsg (bfd_get_error (), matching));

  if ((bfd_get_file_flags (abfd) & (EXEC_P | DYNAMIC)) != 0)
    error (_("\"%s\": not in object format."), filename);

  setup_sections_data.last_size = 0;
  setup_sections_data.last_section_first = abfd->sections;
  setup_sections_data.last_prot = -1;
  setup_sections_data.last_max_alignment = 1;
  setup_sections_data.munmap_list_headp = &munmap_list_head;
  make_cleanup (munmap_listp_free_cleanup, &munmap_list_head);
  bfd_map_over_sections (abfd, setup_sections, &setup_sections_data);
  setup_sections (abfd, NULL, &setup_sections_data);

  storage_needed = bfd_get_symtab_upper_bound (abfd);
  if (storage_needed < 0)
    error (_("Cannot read symbols of compiled module \"%s\": %s"),
          filename, bfd_errmsg (bfd_get_error ()));

  /* SYMFILE_VERBOSE is not passed even if FROM_TTY, user is not interested in
     "Reading symbols from ..." message for automatically generated file.  */
  objfile = symbol_file_add_from_bfd (abfd, filename, 0, NULL, 0, NULL);
  cleanups_free_objfile = make_cleanup_free_objfile (objfile);

  func_sym = lookup_global_symbol_from_objfile (objfile,
						GCC_FE_WRAPPER_FUNCTION,
						VAR_DOMAIN).symbol;
  if (func_sym == NULL)
    error (_("Cannot find function \"%s\" in compiled module \"%s\"."),
	   GCC_FE_WRAPPER_FUNCTION, objfile_name (objfile));
  func_type = SYMBOL_TYPE (func_sym);
  if (TYPE_CODE (func_type) != TYPE_CODE_FUNC)
    error (_("Invalid type code %d of function \"%s\" in compiled "
	     "module \"%s\"."),
	   TYPE_CODE (func_type), GCC_FE_WRAPPER_FUNCTION,
	   objfile_name (objfile));

  switch (scope)
    {
    case COMPILE_I_SIMPLE_SCOPE:
      expect_parameters = 1;
      expect_return_type = builtin_type (target_gdbarch ())->builtin_void;
      break;
    case COMPILE_I_RAW_SCOPE:
      expect_parameters = 0;
      expect_return_type = builtin_type (target_gdbarch ())->builtin_void;
      break;
    case COMPILE_I_PRINT_ADDRESS_SCOPE:
    case COMPILE_I_PRINT_VALUE_SCOPE:
      expect_parameters = 2;
      expect_return_type = builtin_type (target_gdbarch ())->builtin_void;
      break;
    default:
      internal_error (__FILE__, __LINE__, _("invalid scope %d"), scope);
    }
  if (TYPE_NFIELDS (func_type) != expect_parameters)
    error (_("Invalid %d parameters of function \"%s\" in compiled "
	     "module \"%s\"."),
	   TYPE_NFIELDS (func_type), GCC_FE_WRAPPER_FUNCTION,
	   objfile_name (objfile));
  if (!types_deeply_equal (expect_return_type, TYPE_TARGET_TYPE (func_type)))
    error (_("Invalid return type of function \"%s\" in compiled "
	    "module \"%s\"."),
	  GCC_FE_WRAPPER_FUNCTION, objfile_name (objfile));

  /* The memory may be later needed
     by bfd_generic_get_relocated_section_contents
     called from default_symfile_relocate.  */
  symbol_table = (asymbol **) obstack_alloc (&objfile->objfile_obstack,
					     storage_needed);
  number_of_symbols = bfd_canonicalize_symtab (abfd, symbol_table);
  if (number_of_symbols < 0)
    error (_("Cannot parse symbols of compiled module \"%s\": %s"),
          filename, bfd_errmsg (bfd_get_error ()));

  missing_symbols = 0;
  for (symp = symbol_table; symp < symbol_table + number_of_symbols; symp++)
    {
      asymbol *sym = *symp;

      if (sym->flags != 0)
	continue;
      sym->flags = BSF_GLOBAL;
      sym->section = bfd_abs_section_ptr;
      if (strcmp (sym->name, "_GLOBAL_OFFSET_TABLE_") == 0)
	{
	  if (compile_debug)
	    fprintf_unfiltered (gdb_stdlog,
				"ELF symbol \"%s\" relocated to zero\n",
				sym->name);

	  /* It seems to be a GCC bug, with -mcmodel=large there should be no
	     need for _GLOBAL_OFFSET_TABLE_.  Together with -fPIE the data
	     remain PC-relative even with _GLOBAL_OFFSET_TABLE_ as zero.  */
	  sym->value = 0;
	  continue;
	}
      bmsym = lookup_minimal_symbol (sym->name, NULL, NULL);
      switch (bmsym.minsym == NULL
	      ? mst_unknown : MSYMBOL_TYPE (bmsym.minsym))
	{
	case mst_text:
	  sym->value = BMSYMBOL_VALUE_ADDRESS (bmsym);
	  if (compile_debug)
	    fprintf_unfiltered (gdb_stdlog,
				"ELF mst_text symbol \"%s\" relocated to %s\n",
				sym->name,
				paddress (target_gdbarch (), sym->value));
	  break;
	case mst_text_gnu_ifunc:
	  sym->value = gnu_ifunc_resolve_addr (target_gdbarch (),
					       BMSYMBOL_VALUE_ADDRESS (bmsym));
	  if (compile_debug)
	    fprintf_unfiltered (gdb_stdlog,
				"ELF mst_text_gnu_ifunc symbol \"%s\" "
				"relocated to %s\n",
				sym->name,
				paddress (target_gdbarch (), sym->value));
	  break;
	default:
	  warning (_("Could not find symbol \"%s\" "
		     "for compiled module \"%s\"."),
		   sym->name, filename);
	  missing_symbols++;
	}
    }
  if (missing_symbols)
    error (_("%ld symbols were missing, cannot continue."), missing_symbols);

  bfd_map_over_sections (abfd, copy_sections, symbol_table);

  regs_type = get_regs_type (func_sym, objfile);
  if (regs_type == NULL)
    regs_addr = 0;
  else
    {
      /* Use read-only non-executable memory protection.  */
      regs_addr = gdbarch_infcall_mmap (target_gdbarch (),
					TYPE_LENGTH (regs_type),
					GDB_MMAP_PROT_READ);
      gdb_assert (regs_addr != 0);
      munmap_list_add (&munmap_list_head, regs_addr, TYPE_LENGTH (regs_type));
      if (compile_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "allocated %s bytes at %s for registers\n",
			    paddress (target_gdbarch (),
				      TYPE_LENGTH (regs_type)),
			    paddress (target_gdbarch (), regs_addr));
      store_regs (regs_type, regs_addr);
    }

  if (scope == COMPILE_I_PRINT_ADDRESS_SCOPE
      || scope == COMPILE_I_PRINT_VALUE_SCOPE)
    {
      out_value_type = get_out_value_type (func_sym, objfile, scope);
      if (out_value_type == NULL)
	{
	  do_cleanups (cleanups);
	  return NULL;
	}
      check_typedef (out_value_type);
      out_value_addr = gdbarch_infcall_mmap (target_gdbarch (),
					     TYPE_LENGTH (out_value_type),
					     (GDB_MMAP_PROT_READ
					      | GDB_MMAP_PROT_WRITE));
      gdb_assert (out_value_addr != 0);
      munmap_list_add (&munmap_list_head, out_value_addr,
		       TYPE_LENGTH (out_value_type));
      if (compile_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "allocated %s bytes at %s for printed value\n",
			    paddress (target_gdbarch (),
				      TYPE_LENGTH (out_value_type)),
			    paddress (target_gdbarch (), out_value_addr));
    }

  discard_cleanups (cleanups_free_objfile);

  retval = XNEW (struct compile_module);
  retval->objfile = objfile;
  retval->source_file = xstrdup (source_file);
  retval->func_sym = func_sym;
  retval->regs_addr = regs_addr;
  retval->scope = scope;
  retval->scope_data = scope_data;
  retval->out_value_type = out_value_type;
  retval->out_value_addr = out_value_addr;

  /* CLEANUPS will free MUNMAP_LIST_HEAD.  */
  retval->munmap_list_head = munmap_list_head;
  munmap_list_head = NULL;

  do_cleanups (cleanups);

  return retval;
}
