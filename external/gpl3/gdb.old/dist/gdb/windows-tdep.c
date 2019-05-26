/* Copyright (C) 2008-2017 Free Software Foundation, Inc.

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
#include "windows-tdep.h"
#include "gdb_obstack.h"
#include "xml-support.h"
#include "gdbarch.h"
#include "target.h"
#include "value.h"
#include "inferior.h"
#include "command.h"
#include "gdbcmd.h"
#include "gdbthread.h"
#include "objfiles.h"
#include "symfile.h"
#include "coff-pe-read.h"
#include "gdb_bfd.h"
#include "complaints.h"
#include "solib.h"
#include "solib-target.h"
#include "gdbcore.h"

struct cmd_list_element *info_w32_cmdlist;

typedef struct thread_information_block_32
  {
    uint32_t current_seh;			/* %fs:0x0000 */
    uint32_t current_top_of_stack; 		/* %fs:0x0004 */
    uint32_t current_bottom_of_stack;		/* %fs:0x0008 */
    uint32_t sub_system_tib;			/* %fs:0x000c */
    uint32_t fiber_data;			/* %fs:0x0010 */
    uint32_t arbitrary_data_slot;		/* %fs:0x0014 */
    uint32_t linear_address_tib;		/* %fs:0x0018 */
    uint32_t environment_pointer;		/* %fs:0x001c */
    uint32_t process_id;			/* %fs:0x0020 */
    uint32_t current_thread_id;			/* %fs:0x0024 */
    uint32_t active_rpc_handle;			/* %fs:0x0028 */
    uint32_t thread_local_storage;		/* %fs:0x002c */
    uint32_t process_environment_block;		/* %fs:0x0030 */
    uint32_t last_error_number;			/* %fs:0x0034 */
  }
thread_information_32;

typedef struct thread_information_block_64
  {
    uint64_t current_seh;			/* %gs:0x0000 */
    uint64_t current_top_of_stack; 		/* %gs:0x0008 */
    uint64_t current_bottom_of_stack;		/* %gs:0x0010 */
    uint64_t sub_system_tib;			/* %gs:0x0018 */
    uint64_t fiber_data;			/* %gs:0x0020 */
    uint64_t arbitrary_data_slot;		/* %gs:0x0028 */
    uint64_t linear_address_tib;		/* %gs:0x0030 */
    uint64_t environment_pointer;		/* %gs:0x0038 */
    uint64_t process_id;			/* %gs:0x0040 */
    uint64_t current_thread_id;			/* %gs:0x0048 */
    uint64_t active_rpc_handle;			/* %gs:0x0050 */
    uint64_t thread_local_storage;		/* %gs:0x0058 */
    uint64_t process_environment_block;		/* %gs:0x0060 */
    uint64_t last_error_number;			/* %gs:0x0068 */
  }
thread_information_64;


static const char* TIB_NAME[] =
  {
    " current_seh                 ",	/* %fs:0x0000 */
    " current_top_of_stack        ", 	/* %fs:0x0004 */
    " current_bottom_of_stack     ",	/* %fs:0x0008 */
    " sub_system_tib              ",	/* %fs:0x000c */
    " fiber_data                  ",	/* %fs:0x0010 */
    " arbitrary_data_slot         ",	/* %fs:0x0014 */
    " linear_address_tib          ",	/* %fs:0x0018 */
    " environment_pointer         ",	/* %fs:0x001c */
    " process_id                  ",	/* %fs:0x0020 */
    " current_thread_id           ",	/* %fs:0x0024 */
    " active_rpc_handle           ",	/* %fs:0x0028 */
    " thread_local_storage        ",	/* %fs:0x002c */
    " process_environment_block   ",	/* %fs:0x0030 */
    " last_error_number           "	/* %fs:0x0034 */
  };

static const int MAX_TIB32 =
  sizeof (thread_information_32) / sizeof (uint32_t);
static const int MAX_TIB64 =
  sizeof (thread_information_64) / sizeof (uint64_t);
static const int FULL_TIB_SIZE = 0x1000;

static int maint_display_all_tib = 0;

/* Define Thread Local Base pointer type.  */

static struct type *
windows_get_tlb_type (struct gdbarch *gdbarch)
{
  static struct gdbarch *last_gdbarch = NULL;
  static struct type *last_tlb_type = NULL;
  struct type *dword_ptr_type, *dword32_type, *void_ptr_type;
  struct type *peb_ldr_type, *peb_ldr_ptr_type;
  struct type *peb_type, *peb_ptr_type, *list_type;
  struct type *module_list_ptr_type;
  struct type *tib_type, *seh_type, *tib_ptr_type, *seh_ptr_type;

  /* Do not rebuild type if same gdbarch as last time.  */
  if (last_tlb_type && last_gdbarch == gdbarch)
    return last_tlb_type;
  
  dword_ptr_type = arch_integer_type (gdbarch, gdbarch_ptr_bit (gdbarch),
				 1, "DWORD_PTR");
  dword32_type = arch_integer_type (gdbarch, 32,
				 1, "DWORD32");
  void_ptr_type = lookup_pointer_type (builtin_type (gdbarch)->builtin_void);

  /* list entry */

  list_type = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
  TYPE_NAME (list_type) = xstrdup ("list");

  module_list_ptr_type = void_ptr_type;

  append_composite_type_field (list_type, "forward_list",
			       module_list_ptr_type);
  append_composite_type_field (list_type, "backward_list",
			       module_list_ptr_type);

  /* Structured Exception Handler */

  seh_type = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
  TYPE_NAME (seh_type) = xstrdup ("seh");

  seh_ptr_type = arch_type (gdbarch, TYPE_CODE_PTR,
			    TYPE_LENGTH (void_ptr_type), NULL);
  TYPE_TARGET_TYPE (seh_ptr_type) = seh_type;

  append_composite_type_field (seh_type, "next_seh", seh_ptr_type);
  append_composite_type_field (seh_type, "handler",
			       builtin_type (gdbarch)->builtin_func_ptr);

  /* struct _PEB_LDR_DATA */
  peb_ldr_type = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
  TYPE_NAME (peb_ldr_type) = xstrdup ("peb_ldr_data");

  append_composite_type_field (peb_ldr_type, "length", dword32_type);
  append_composite_type_field (peb_ldr_type, "initialized", dword32_type);
  append_composite_type_field (peb_ldr_type, "ss_handle", void_ptr_type);
  append_composite_type_field (peb_ldr_type, "in_load_order", list_type);
  append_composite_type_field (peb_ldr_type, "in_memory_order", list_type);
  append_composite_type_field (peb_ldr_type, "in_init_order", list_type);
  append_composite_type_field (peb_ldr_type, "entry_in_progress",
			       void_ptr_type);
  peb_ldr_ptr_type = arch_type (gdbarch, TYPE_CODE_PTR,
			    TYPE_LENGTH (void_ptr_type), NULL);
  TYPE_TARGET_TYPE (peb_ldr_ptr_type) = peb_ldr_type;


  /* struct process environment block */
  peb_type = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
  TYPE_NAME (peb_type) = xstrdup ("peb");

  /* First bytes contain several flags.  */
  append_composite_type_field (peb_type, "flags", dword_ptr_type);
  append_composite_type_field (peb_type, "mutant", void_ptr_type);
  append_composite_type_field (peb_type, "image_base_address", void_ptr_type);
  append_composite_type_field (peb_type, "ldr", peb_ldr_ptr_type);
  append_composite_type_field (peb_type, "process_parameters", void_ptr_type);
  append_composite_type_field (peb_type, "sub_system_data", void_ptr_type);
  append_composite_type_field (peb_type, "process_heap", void_ptr_type);
  append_composite_type_field (peb_type, "fast_peb_lock", void_ptr_type);
  peb_ptr_type = arch_type (gdbarch, TYPE_CODE_PTR,
			    TYPE_LENGTH (void_ptr_type), NULL);
  TYPE_TARGET_TYPE (peb_ptr_type) = peb_type;


  /* struct thread information block */
  tib_type = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
  TYPE_NAME (tib_type) = xstrdup ("tib");

  /* uint32_t current_seh;			%fs:0x0000 */
  append_composite_type_field (tib_type, "current_seh", seh_ptr_type);
  /* uint32_t current_top_of_stack; 		%fs:0x0004 */
  append_composite_type_field (tib_type, "current_top_of_stack",
			       void_ptr_type);
  /* uint32_t current_bottom_of_stack;		%fs:0x0008 */
  append_composite_type_field (tib_type, "current_bottom_of_stack",
			       void_ptr_type);
  /* uint32_t sub_system_tib;			%fs:0x000c */
  append_composite_type_field (tib_type, "sub_system_tib", void_ptr_type);

  /* uint32_t fiber_data;			%fs:0x0010 */
  append_composite_type_field (tib_type, "fiber_data", void_ptr_type);
  /* uint32_t arbitrary_data_slot;		%fs:0x0014 */
  append_composite_type_field (tib_type, "arbitrary_data_slot", void_ptr_type);
  /* uint32_t linear_address_tib;		%fs:0x0018 */
  append_composite_type_field (tib_type, "linear_address_tib", void_ptr_type);
  /* uint32_t environment_pointer;		%fs:0x001c */
  append_composite_type_field (tib_type, "environment_pointer", void_ptr_type);
  /* uint32_t process_id;			%fs:0x0020 */
  append_composite_type_field (tib_type, "process_id", dword_ptr_type);
  /* uint32_t current_thread_id;		%fs:0x0024 */
  append_composite_type_field (tib_type, "thread_id", dword_ptr_type);
  /* uint32_t active_rpc_handle;		%fs:0x0028 */
  append_composite_type_field (tib_type, "active_rpc_handle", dword_ptr_type);
  /* uint32_t thread_local_storage;		%fs:0x002c */
  append_composite_type_field (tib_type, "thread_local_storage",
			       void_ptr_type);
  /* uint32_t process_environment_block;	%fs:0x0030 */
  append_composite_type_field (tib_type, "process_environment_block",
			       peb_ptr_type);
  /* uint32_t last_error_number;		%fs:0x0034 */
  append_composite_type_field (tib_type, "last_error_number", dword_ptr_type);

  tib_ptr_type = arch_type (gdbarch, TYPE_CODE_PTR,
			    TYPE_LENGTH (void_ptr_type), NULL);
  TYPE_TARGET_TYPE (tib_ptr_type) = tib_type;

  last_tlb_type = tib_ptr_type;
  last_gdbarch = gdbarch;

  return tib_ptr_type;
}

/* The $_tlb convenience variable is a bit special.  We don't know
   for sure the type of the value until we actually have a chance to
   fetch the data.  The type can change depending on gdbarch, so it is
   also dependent on which thread you have selected.  */

/* This function implements the lval_computed support for reading a
   $_tlb value.  */

static void
tlb_value_read (struct value *val)
{
  CORE_ADDR tlb;
  struct type *type = check_typedef (value_type (val));

  if (!target_get_tib_address (inferior_ptid, &tlb))
    error (_("Unable to read tlb"));
  store_typed_address (value_contents_raw (val), type, tlb);
}

/* This function implements the lval_computed support for writing a
   $_tlb value.  */

static void
tlb_value_write (struct value *v, struct value *fromval)
{
  error (_("Impossible to change the Thread Local Base"));
}

static const struct lval_funcs tlb_value_funcs =
  {
    tlb_value_read,
    tlb_value_write
  };


/* Return a new value with the correct type for the tlb object of
   the current thread using architecture GDBARCH.  Return a void value
   if there's no object available.  */

static struct value *
tlb_make_value (struct gdbarch *gdbarch, struct internalvar *var, void *ignore)
{
  if (target_has_stack && !ptid_equal (inferior_ptid, null_ptid))
    {
      struct type *type = windows_get_tlb_type (gdbarch);
      return allocate_computed_value (type, &tlb_value_funcs, NULL);
    }

  return allocate_value (builtin_type (gdbarch)->builtin_void);
}


/* Display thread information block of a given thread.  */

static int
display_one_tib (ptid_t ptid)
{
  gdb_byte *tib = NULL;
  gdb_byte *index;
  CORE_ADDR thread_local_base;
  ULONGEST i, val, max, max_name, size, tib_size;
  ULONGEST sizeof_ptr = gdbarch_ptr_bit (target_gdbarch ());
  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());

  if (sizeof_ptr == 64)
    {
      size = sizeof (uint64_t);
      tib_size = sizeof (thread_information_64);
      max = MAX_TIB64;
    }
  else
    {
      size = sizeof (uint32_t);
      tib_size = sizeof (thread_information_32);
      max = MAX_TIB32;
    }

  max_name = max;

  if (maint_display_all_tib)
    {
      tib_size = FULL_TIB_SIZE;
      max = tib_size / size;
    }
  
  tib = (gdb_byte *) alloca (tib_size);

  if (target_get_tib_address (ptid, &thread_local_base) == 0)
    {
      printf_filtered (_("Unable to get thread local base for %s\n"),
	target_pid_to_str (ptid));
      return -1;
    }

  if (target_read (&current_target, TARGET_OBJECT_MEMORY,
		   NULL, tib, thread_local_base, tib_size) != tib_size)
    {
      printf_filtered (_("Unable to read thread information "
			 "block for %s at address %s\n"),
	target_pid_to_str (ptid), 
	paddress (target_gdbarch (), thread_local_base));
      return -1;
    }

  printf_filtered (_("Thread Information Block %s at %s\n"),
		   target_pid_to_str (ptid),
		   paddress (target_gdbarch (), thread_local_base));

  index = (gdb_byte *) tib;

  /* All fields have the size of a pointer, this allows to iterate 
     using the same for loop for both layouts.  */
  for (i = 0; i < max; i++)
    {
      val = extract_unsigned_integer (index, size, byte_order);
      if (i < max_name)
	printf_filtered (_("%s is 0x%s\n"), TIB_NAME[i], phex (val, size));
      else if (val != 0)
	printf_filtered (_("TIB[0x%s] is 0x%s\n"), phex (i * size, 2),
			 phex (val, size));
      index += size;
    } 
  return 1;  
}

/* Display thread information block of the current thread.  */

static void
display_tib (char * args, int from_tty)
{
  if (!ptid_equal (inferior_ptid, null_ptid))
    display_one_tib (inferior_ptid);
}

void
windows_xfer_shared_library (const char* so_name, CORE_ADDR load_addr,
			     struct gdbarch *gdbarch, struct obstack *obstack)
{
  char *p;
  CORE_ADDR text_offset;

  obstack_grow_str (obstack, "<library name=\"");
  p = xml_escape_text (so_name);
  obstack_grow_str (obstack, p);
  xfree (p);
  obstack_grow_str (obstack, "\"><segment address=\"");
  gdb_bfd_ref_ptr dll (gdb_bfd_open (so_name, gnutarget, -1));
  /* The following calls are OK even if dll is NULL.
     The default value 0x1000 is returned by pe_text_section_offset
     in that case.  */
  text_offset = pe_text_section_offset (dll.get ());
  obstack_grow_str (obstack, paddress (gdbarch, load_addr + text_offset));
  obstack_grow_str (obstack, "\"/></library>");
}

/* Implement the "iterate_over_objfiles_in_search_order" gdbarch
   method.  It searches all objfiles, starting with CURRENT_OBJFILE
   first (if not NULL).

   On Windows, the system behaves a little differently when two
   objfiles each define a global symbol using the same name, compared
   to other platforms such as GNU/Linux for instance.  On GNU/Linux,
   all instances of the symbol effectively get merged into a single
   one, but on Windows, they remain distinct.

   As a result, it usually makes sense to start global symbol searches
   with the current objfile before expanding it to all other objfiles.
   This helps for instance when a user debugs some code in a DLL that
   refers to a global variable defined inside that DLL.  When trying
   to print the value of that global variable, it would be unhelpful
   to print the value of another global variable defined with the same
   name, but in a different DLL.  */

static void
windows_iterate_over_objfiles_in_search_order
  (struct gdbarch *gdbarch,
   iterate_over_objfiles_in_search_order_cb_ftype *cb,
   void *cb_data, struct objfile *current_objfile)
{
  int stop;
  struct objfile *objfile;

  if (current_objfile)
    {
      stop = cb (current_objfile, cb_data);
      if (stop)
	return;
    }

  ALL_OBJFILES (objfile)
    {
      if (objfile != current_objfile)
	{
	  stop = cb (objfile, cb_data);
	  if (stop)
	    return;
	}
    }
}

static void
show_maint_show_all_tib (struct ui_file *file, int from_tty,
		struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Show all non-zero elements of "
			    "Thread Information Block is %s.\n"), value);
}

static void
info_w32_command (char *args, int from_tty)
{
  help_list (info_w32_cmdlist, "info w32 ", class_info, gdb_stdout);
}

static int w32_prefix_command_valid = 0;
void
init_w32_command_list (void)
{
  if (!w32_prefix_command_valid)
    {
      add_prefix_cmd ("w32", class_info, info_w32_command,
		      _("Print information specific to Win32 debugging."),
		      &info_w32_cmdlist, "info w32 ", 0, &infolist);
      w32_prefix_command_valid = 1;
    }
}

/* To be called from the various GDB_OSABI_CYGWIN handlers for the
   various Windows architectures and machine types.  */

void
windows_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  set_gdbarch_wchar_bit (gdbarch, 16);
  set_gdbarch_wchar_signed (gdbarch, 0);

  /* Canonical paths on this target look like
     `c:\Program Files\Foo App\mydll.dll', for example.  */
  set_gdbarch_has_dos_based_file_system (gdbarch, 1);

  set_gdbarch_iterate_over_objfiles_in_search_order
    (gdbarch, windows_iterate_over_objfiles_in_search_order);

  set_solib_ops (gdbarch, &solib_target_so_ops);
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_windows_tdep;

/* Implementation of `tlb' variable.  */

static const struct internalvar_funcs tlb_funcs =
{
  tlb_make_value,
  NULL,
  NULL
};

void
_initialize_windows_tdep (void)
{
  init_w32_command_list ();
  add_cmd ("thread-information-block", class_info, display_tib,
	   _("Display thread information block."),
	   &info_w32_cmdlist);
  add_alias_cmd ("tib", "thread-information-block", class_info, 1,
		 &info_w32_cmdlist);

  add_setshow_boolean_cmd ("show-all-tib", class_maintenance,
			   &maint_display_all_tib, _("\
Set whether to display all non-zero fields of thread information block."), _("\
Show whether to display all non-zero fields of thread information block."), _("\
Use \"on\" to enable, \"off\" to disable.\n\
If enabled, all non-zero fields of thread information block are displayed,\n\
even if their meaning is unknown."),
			   NULL,
			   show_maint_show_all_tib,
			   &maintenance_set_cmdlist,
			   &maintenance_show_cmdlist);

  /* Explicitly create without lookup, since that tries to create a
     value with a void typed value, and when we get here, gdbarch
     isn't initialized yet.  At this point, we're quite sure there
     isn't another convenience variable of the same name.  */
  create_internalvar_type_lazy ("_tlb", &tlb_funcs, NULL);
}
