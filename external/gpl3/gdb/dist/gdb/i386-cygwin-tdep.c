/* Target-dependent code for Cygwin running on i386's, for GDB.

   Copyright (C) 2003-2014 Free Software Foundation, Inc.

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
#include "osabi.h"
#include <string.h>
#include "i386-tdep.h"
#include "windows-tdep.h"
#include "regset.h"
#include "gdb_obstack.h"
#include "xml-support.h"
#include "gdbcore.h"
#include "inferior.h"

/* Core file support.  */

/* This vector maps GDB's idea of a register's number into an address
   in the windows exception context vector.  */

static int i386_windows_gregset_reg_offset[] =
{
  176, /* eax */
  172, /* ecx */
  168, /* edx */
  164, /* ebx */

  196, /* esp */
  180, /* ebp */
  160, /* esi */
  156, /* edi */

  184, /* eip */
  192, /* eflags */
  188, /* cs */
  200, /* ss */

  152, /* ds */
  148, /* es */
  144, /* fs */
  140, /* gs */

  56, /* FloatSave.RegisterArea[0 * 10] */
  66, /* FloatSave.RegisterArea[1 * 10] */
  76, /* FloatSave.RegisterArea[2 * 10] */
  86, /* FloatSave.RegisterArea[3 * 10] */
  96, /* FloatSave.RegisterArea[4 * 10] */
  106, /* FloatSave.RegisterArea[5 * 10] */
  116, /* FloatSave.RegisterArea[6 * 10] */
  126, /* FloatSave.RegisterArea[7 * 10] */

  28, /* FloatSave.ControlWord */
  32, /* FloatSave.StatusWord */
  36, /* FloatSave.TagWord */
  44, /* FloatSave.ErrorSelector */
  40, /* FloatSave.ErrorOffset */
  52, /* FloatSave.DataSelector */
  48, /* FloatSave.DataOffset */
  44, /* FloatSave.ErrorSelector */

  /* XMM0-7 */
  364, /* ExtendedRegisters[10*16] */
  380, /* ExtendedRegisters[11*16] */
  396, /* ExtendedRegisters[12*16] */
  412, /* ExtendedRegisters[13*16] */
  428, /* ExtendedRegisters[14*16] */
  444, /* ExtendedRegisters[15*16] */
  460, /* ExtendedRegisters[16*16] */
  476, /* ExtendedRegisters[17*16] */

  /* MXCSR */
  228 /* ExtendedRegisters[24] */
};

#define I386_WINDOWS_SIZEOF_GREGSET 716

/* Return the appropriate register set for the core section identified
   by SECT_NAME and SECT_SIZE.  */

static const struct regset *
i386_windows_regset_from_core_section (struct gdbarch *gdbarch,
				     const char *sect_name, size_t sect_size)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  if (strcmp (sect_name, ".reg") == 0
      && sect_size == I386_WINDOWS_SIZEOF_GREGSET)
    {
      if (tdep->gregset == NULL)
        tdep->gregset = regset_alloc (gdbarch, i386_supply_gregset,
                                      i386_collect_gregset);
      return tdep->gregset;
    }

  return NULL;
}

struct cpms_data
{
  struct gdbarch *gdbarch;
  struct obstack *obstack;
  int module_count;
};

static void
core_process_module_section (bfd *abfd, asection *sect, void *obj)
{
  struct cpms_data *data = obj;
  enum bfd_endian byte_order = gdbarch_byte_order (data->gdbarch);

  char *module_name;
  size_t module_name_size;
  CORE_ADDR base_addr;

  gdb_byte *buf = NULL;

  if (strncmp (sect->name, ".module", 7) != 0)
    return;

  buf = xmalloc (bfd_get_section_size (sect) + 1);
  if (!buf)
    {
      printf_unfiltered ("memory allocation failed for %s\n", sect->name);
      goto out;
    }
  if (!bfd_get_section_contents (abfd, sect,
				 buf, 0, bfd_get_section_size (sect)))
    goto out;



  /* A DWORD (data_type) followed by struct windows_core_module_info.  */

  base_addr =
    extract_unsigned_integer (buf + 4, 4, byte_order);

  module_name_size =
    extract_unsigned_integer (buf + 8, 4, byte_order);

  if (12 + module_name_size > bfd_get_section_size (sect))
    goto out;
  module_name = (char *) buf + 12;

  /* The first module is the .exe itself.  */
  if (data->module_count != 0)
    windows_xfer_shared_library (module_name, base_addr,
				 data->gdbarch, data->obstack);
  data->module_count++;

out:
  if (buf)
    xfree (buf);
  return;
}

static LONGEST
windows_core_xfer_shared_libraries (struct gdbarch *gdbarch,
				  gdb_byte *readbuf,
				  ULONGEST offset, LONGEST len)
{
  struct obstack obstack;
  const char *buf;
  LONGEST len_avail;
  struct cpms_data data = { gdbarch, &obstack, 0 };

  obstack_init (&obstack);
  obstack_grow_str (&obstack, "<library-list>\n");
  bfd_map_over_sections (core_bfd,
			 core_process_module_section,
			 &data);
  obstack_grow_str0 (&obstack, "</library-list>\n");

  buf = obstack_finish (&obstack);
  len_avail = strlen (buf);
  if (offset >= len_avail)
    return 0;

  if (len > len_avail - offset)
    len = len_avail - offset;
  memcpy (readbuf, buf + offset, len);

  obstack_free (&obstack, NULL);
  return len;
}

/* This is how we want PTIDs from core files to be printed.  */

static char *
i386_windows_core_pid_to_str (struct gdbarch *gdbarch, ptid_t ptid)
{
  static char buf[80];

  if (ptid_get_lwp (ptid) != 0)
    {
      snprintf (buf, sizeof (buf), "Thread 0x%lx", ptid_get_lwp (ptid));
      return buf;
    }

  return normal_pid_to_str (ptid);
}

static CORE_ADDR
i386_cygwin_skip_trampoline_code (struct frame_info *frame, CORE_ADDR pc)
{
  return i386_pe_skip_trampoline_code (frame, pc, NULL);
}

static const char *
i386_cygwin_auto_wide_charset (void)
{
  return "UTF-16";
}

static void
i386_cygwin_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  windows_init_abi (info, gdbarch);

  set_gdbarch_skip_trampoline_code (gdbarch, i386_cygwin_skip_trampoline_code);

  set_gdbarch_skip_main_prologue (gdbarch, i386_skip_main_prologue);

  tdep->struct_return = reg_struct_return;

  tdep->gregset_reg_offset = i386_windows_gregset_reg_offset;
  tdep->gregset_num_regs = ARRAY_SIZE (i386_windows_gregset_reg_offset);
  tdep->sizeof_gregset = I386_WINDOWS_SIZEOF_GREGSET;

  /* Core file support.  */
  set_gdbarch_regset_from_core_section
    (gdbarch, i386_windows_regset_from_core_section);
  set_gdbarch_core_xfer_shared_libraries
    (gdbarch, windows_core_xfer_shared_libraries);
  set_gdbarch_core_pid_to_str (gdbarch, i386_windows_core_pid_to_str);

  set_gdbarch_auto_wide_charset (gdbarch, i386_cygwin_auto_wide_charset);
}

static enum gdb_osabi
i386_cygwin_osabi_sniffer (bfd *abfd)
{
  char *target_name = bfd_get_target (abfd);

  if (strcmp (target_name, "pei-i386") == 0)
    return GDB_OSABI_CYGWIN;

  /* Cygwin uses elf core dumps.  Do not claim all ELF executables,
     check whether there is a .reg section of proper size.  */
  if (strcmp (target_name, "elf32-i386") == 0)
    {
      asection *section = bfd_get_section_by_name (abfd, ".reg");
      if (section
	  && bfd_section_size (abfd, section) == I386_WINDOWS_SIZEOF_GREGSET)
	return GDB_OSABI_CYGWIN;
    }

  return GDB_OSABI_UNKNOWN;
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
void _initialize_i386_cygwin_tdep (void);

void
_initialize_i386_cygwin_tdep (void)
{
  gdbarch_register_osabi_sniffer (bfd_arch_i386, bfd_target_coff_flavour,
                                  i386_cygwin_osabi_sniffer);

  /* Cygwin uses elf core dumps.  */
  gdbarch_register_osabi_sniffer (bfd_arch_i386, bfd_target_elf_flavour,
                                  i386_cygwin_osabi_sniffer);

  gdbarch_register_osabi (bfd_arch_i386, 0, GDB_OSABI_CYGWIN,
                          i386_cygwin_init_abi);
}
